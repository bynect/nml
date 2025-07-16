#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"
#include "decl.h"
#include "env.h"

typedef enum {
    OFF_ARG,
    OFF_LET,
    OFF_FV,
    OFF_GLOB,
} offset_type_t;

#define OFF_GET(o)    (((o) >> 56) & 0xFF)
#define OFF_SET(o, t) (((uintptr_t)(t) << 56) | (o))
#define OFF_CLS(o)    ((o) & ~((uintptr_t)0xFF << 56))

void compile_init(compile_t *comp, FILE *file)
{
    comp->file = file;
    comp->lambda_id = 0;
    comp->init_id = 0;
    comp->let_n = 0;
    comp->env = NULL;
    comp->main = NULL;
}

static bool compile_emit_expr(compile_t *comp, expr_t *expr);

static bool compile_emit_var(compile_t *comp, expr_var_t *var)
{
    uintptr_t offset;
    if (env_find(comp->env, var->name, (intptr_t *)&offset) < 0) {
        printf("Unbound reference to '%s'\n", var->name);
        return false;
    }

    switch (OFF_GET(offset)) {
        case OFF_ARG:
            fprintf(comp->file,
                    "\tmovq %%r14, %%r12\t\t#arg %s\n",
                    var->name);
            break;

        case OFF_LET:
            fprintf(comp->file,
                    "\tmovq -%lu(%%rbp), %%r12\t\t#let %s\n",
                    OFF_CLS(offset),
                    var->name);
            break;

        case OFF_FV:
            fprintf(comp->file,
                    "\tmovq %lu(%%r13), %%r12\t\t#fv %s\n",
                    OFF_CLS(offset),
                    var->name);
            break;

        case OFF_GLOB:
            fprintf(comp->file,
                    "\tmovq glob_%lu(%%rip), %%r12\t\t#glob %s\n",
                    OFF_CLS(offset),
                    var->name);
            break;

        default:
            return false;
    }

    return true;
}

static bool compile_freevars(compile_t *comp, expr_t *expr, env_t **env)
{
    switch (expr->tag) {
        case EXPR_LIT:
            break;

        case EXPR_VAR: {
            expr_var_t *var = (expr_var_t *)expr;
            *env = env_update(*env, var->name, OFF_SET(0, OFF_ARG));
            break;
        }

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            if (!compile_freevars(comp, lam->body, env))
                return false;

            *env = env_remove(*env, lam->bound);
            break;
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            return compile_freevars(comp, app->fun, env)
                && compile_freevars(comp, app->arg, env);
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            if (!compile_freevars(comp, let->body, env))
                return false;

            *env = env_remove(*env, let->bound);
            if (!compile_freevars(comp, let->value, env))
                return false;

            comp->let_n++;
            break;
        }
    }
    return true;
}

static bool compile_emit_lambda(compile_t *comp, expr_lambda_t *lam)
{
    if (lam->id != NULL) {
        size_t n_freevars = env_length(lam->freevars);

        fprintf(comp->file,
                "\tmovq $%ld, %%rdi\n"
                "\tcall malloc\n"
                "\tmovq %%rax, %%r15\n"
                "\tleaq %s(%%rip), %%rax\n"
                "\tmovq %%rax, (%%r15)\n",
                (n_freevars + 1) * 8,
                lam->id);

        for (env_t *env = lam->freevars; env; env = env->next) {
            expr_var_t var = { 0 };
            var.name = env->name;

            if (!compile_emit_var(comp, &var))
                return false;

            if (OFF_GET(env->value) != OFF_FV)
                return false;

            fprintf(comp->file,
                    "\tmovq %%r12, %ld(%%r15)\n",
                    OFF_CLS(env->value));
        }

        fputs("\tmovq %r15, %r12\n\n", comp->file);
        return true;
    }

    char *id = malloc(16);
    snprintf(id, 16, "lambda_%u", comp->lambda_id++);
    lam->id = id;

    comp->let_n = 0;
    env_t *freevars = NULL;
    if (!compile_freevars(comp, (expr_t *)lam, &freevars)) {
        printf("Failed to get freevars\n");
        return false;
    }

    size_t let_n = comp->let_n;
    comp->let_n = 0;

    size_t offset = 8;
    for (env_t *fv = freevars; fv; fv = fv->next) {
        fv->value = OFF_SET(offset, OFF_FV);
        offset += 8;
    }

    env_t *env = comp->env;
    comp->env = env_append(freevars, lam->bound, OFF_SET(0, OFF_ARG));

    fprintf(comp->file, "%s:\n", id);
    if (let_n) {
        fprintf(comp->file,
                "\tpushq %%rbp\n"
                "\tmovq %%rsp, %%rbp\n"
                "\tsubq $%ld, %%rsp\n"
                "\n",
                let_n * 8);
    }

    if (!compile_emit_expr(comp, lam->body))
        return false;

    lam->freevars = env_clear(comp->env, freevars);
    comp->env = env;

    if (let_n)
        fputs("\tleave\n", comp->file);

    fputs("\tret\n\n", comp->file);
    return true;
}

static bool compile_emit_lit(compile_t *comp, expr_lit_t *lit)
{
    fprintf(comp->file, "\tmovq $%ld, %%r12\n", lit->value);
    return true;
}

static bool compile_emit_apply(compile_t *comp, expr_apply_t *app)
{
    if (!compile_emit_expr(comp, app->fun))
        return false;

    fputs("\tpushq %r12\n", comp->file);
    if (!compile_emit_expr(comp, app->arg))
        return false;

    fputs("\tmovq %r12, %r14\n"
          "\tpopq %r13\n"
          "\tcall *(%r13)\n"
          "\n",
          comp->file);

    return true;
}

static bool compile_emit_let(compile_t *comp, expr_let_t *let)
{
    if (!compile_emit_expr(comp, let->value))
        return false;

    env_t *env = comp->env;
    comp->env = env_append(env, let->bound, OFF_SET(++comp->let_n * 8, OFF_LET));

    fprintf(comp->file, "\tmovq %%r12, -%ld(%%rbp)\n", comp->let_n * 8);
    if (!compile_emit_expr(comp, let->body))
        return false;

    comp->env = env_clear(comp->env, env);
    comp->let_n--;
    return true;
}

static bool compile_emit_expr(compile_t *comp, expr_t *expr)
{
    switch (expr->tag) {
        case EXPR_LIT:
            return compile_emit_lit(comp, (expr_lit_t *)expr);

        case EXPR_VAR:
            return compile_emit_var(comp, (expr_var_t *)expr);

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            return compile_emit_lambda(comp, lam);
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            return compile_emit_apply(comp, app);
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            return compile_emit_let(comp, let);
        }
    }
    return true;
}

static bool compile_lambdas(compile_t *comp, expr_t *expr)
{
    switch (expr->tag) {
        case EXPR_LIT:
        case EXPR_VAR:
            break;

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            env_t *env = comp->env;

            comp->env = env_append(env, lam->bound, OFF_SET(0, OFF_ARG));
            if (!compile_lambdas(comp, lam->body))
                return false;

            if (!compile_emit_lambda(comp, lam))
                return false;

            comp->env = env_clear(comp->env, env);
            return true;
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            return compile_lambdas(comp, app->fun)
                && compile_lambdas(comp, app->arg);
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            return compile_lambdas(comp, let->value)
                && compile_lambdas(comp, let->body);
        }
    }

    return true;
}

bool compile_decl(compile_t *comp, decl_t *decl)
{
    if (decl->tag != DECL_LET)
        return false;

    decl_let_t *let = (decl_let_t *)decl;

    if (!strcmp(let->bound, "main"))
        comp->main = let;

    if (!compile_lambdas(comp, let->value))
        return false;

    let->id = comp->init_id++;
    fprintf(comp->file, "init_%u:\n", let->id);

    if (!compile_emit_expr(comp, let->value))
        return false;

    fprintf(comp->file,
            "\tmovq %%r12, glob_%u(%%rip)\n"
            "\tret\n"
            "\n",
            let->id);

    comp->env = env_append(comp->env, let->bound, OFF_SET(let->id, OFF_GLOB));
    return true;
}

bool compile_main(compile_t *comp)
{
    if (comp->main == NULL)
        return false;

    fputs(".globl main\n"
          "main:\n"
          "\tpushq %rbp\n"
          "\tmovq %rsp, %rbp\n"
          "\tpushq %r12\n"
          "\tpushq %r13\n"
          "\tpushq %r14\n"
          "\tpushq %r15\n",
          comp->file);

    for (uint32_t i = 0; i < comp->init_id; i++) {
        if (comp->main->id == i) continue;
        fprintf(comp->file, "\tcall init_%u\n", i);
    }

    fprintf(comp->file, "\tcall init_%u\t\t#main\n", comp->main->id);

    fputs("\tmovq %r12, %rsi\n"
          "\tleaq fmt(%rip), %rdi\n"
          "\tcall printf\n"
          "\tpopq %r15\n"
          "\tpopq %r14\n"
          "\tpopq %r13\n"
          "\tpopq %r12\n"
          "\tleave\n"
          "\tret\n"
          "\n"
          ".extern malloc\n"
          "\n"
          ".section .rodata\n"
          "fmt: .asciz \"Result: %ld\\n\"\n"
          "\n"
          ".section .note.GNU-stack,\"\",@progbits\n"
          "\n"
          ".section .bss\n"
          ".align 8\n",
          comp->file);

    for (uint32_t i = 0; i < comp->init_id; i++) {
        fprintf(comp->file, "glob_%u: .skip 8\n", i);
    }

    return true;
}

void compile_free(compile_t *comp)
{
    env_clear(comp->env, NULL);
}
