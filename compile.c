#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "compile.h"
#include "env.h"

void compile_init(compile_t *comp, FILE *file)
{
    comp->file = file;
    comp->lam_id = 0;
    comp->let_n = 0;
    comp->env = NULL;
}

static bool compile_emit_expr(compile_t *comp, expr_t *expr);

static bool compile_emit_var(compile_t *comp, expr_var_t *var)
{
    long offset;
    if (env_find(comp->env, var->name, &offset) < 0) {
        printf("Unbound reference to '%s'\n", var->name);
        return false;
    }

    if (offset == 0) {
        fprintf(comp->file, "\tmovq %%r14, %%r12\t\t#arg %s\n",
                var->name);
    } else if (offset < 0) {
        fprintf(comp->file, "\tmovq %ld(%%rbp), %%r12\t\t#let %s\n",
                offset, var->name);
    } else {
        fprintf(comp->file, "\tmovq %ld(%%r13), %%r12\t\t#fv %s\n",
                offset, var->name);
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
            *env = env_update(*env, var->name, 0);
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

            fprintf(comp->file,
                    "\tmovq %%r12, %ld(%%r15)\n",
                    env->value);
        }

        fputs("\tmovq %r15, %r12\n\n", comp->file);
        return true;
    }

    char *id = malloc(16);
    snprintf(id, 16, "lambda_%ld", comp->lam_id++);
    lam->id = id;

    comp->let_n = 0;
    env_t *freevars = NULL;
    if (!compile_freevars(comp, (expr_t *)lam, &freevars)) {
        printf("Failed to get freevars\n");
        return false;
    }

    size_t let_n = comp->let_n;
    comp->let_n = 0;

    long offset = 8;
    for (env_t *fv = freevars; fv; fv = fv->next) {
        fv->value = offset;
        offset += 8;
    }

    env_t *env = comp->env;
    comp->env = env_append(freevars, lam->bound, 0);

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
    comp->env = env_append(env, let->bound, --comp->let_n * 8);

    fprintf(comp->file, "\tmovq %%r12, %ld(%%rbp)\n", comp->let_n * 8);
    if (!compile_emit_expr(comp, let->body))
        return false;

    comp->env = env_clear(comp->env, env);
    comp->let_n++;
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

            comp->env = env_append(env, lam->bound, 0);

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

bool compile_expr(compile_t *comp, expr_t *expr)
{
    fputs(".extern printf\n"
          ".extern malloc\n"
          "\n",
          comp->file);

    if (!compile_lambdas(comp, expr))
        return false;

    // r12 -> last value
    // r13 -> current closure
    // r14 -> last argument
    // r15 -> temporary

    fputs(".globl main\n"
          "main:\n"
          "\tpushq %rbp\n"
          "\tmovq %rsp, %rbp\n"
          "\tpushq %r12\n"
          "\tpushq %r13\n"
          "\tpushq %r14\n"
          "\tpushq %r15\n"
          "\txorq %r12, %r12\n"
          "\txorq %r13, %r13\n"
          "\n",
          comp->file);

    if (!compile_emit_expr(comp, expr))
        return false;

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
          ".section .rodata\n"
          "fmt: .asciz \"Result: %ld\\n\"\n"
          "\n"
          ".section .note.GNU-stack,\"\",@progbits\n",
          comp->file);

    return true;
}

void compile_free(compile_t *comp)
{
    env_clear(comp->env, NULL);
}
