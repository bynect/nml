#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compile.h"
#include "ast.h"

static env_t *env_append(const char *name, int offset, env_t *tail)
{
    env_t *head = malloc(sizeof(env_t));
    head->name = name;
    head->offset = offset;
    head->next = tail;
    return head;
}

void compile_init(compile_t *comp, FILE *file)
{
    comp->file = file;
    comp->lam_id = 0;
    comp->env = NULL;
}

static bool compile_emit_expr(compile_t *comp, expr_t *expr);

static bool compile_emit_lambda(compile_t *comp, expr_lambda_t *lam)
{
    if (lam->id != NULL) {
        fprintf(comp->file, "\tleaq %s(%%rip), %%rax\n", lam->id);
        return true;
    }

    char *id = malloc(16);
    snprintf(id, 16, "lambda_%d", comp->lam_id++);
    lam->id = id;

    env_t *env = comp->env;
    comp->env = env_append(lam->bound, 8, env);

    fprintf(comp->file, "%s:\n", id);

    if (!compile_emit_expr(comp, lam->body))
        return false;

    fprintf(comp->file, "\tret\n\n");

    do {
        env_t *next = comp->env->next;
        free(comp->env);
        comp->env = next;
    } while (comp->env != env);
    comp->env = env;

    return true;
}

static bool compile_emit_lit(compile_t *comp, expr_lit_t *lit)
{
    fprintf(comp->file, "\tmovq $%ld, %%rax\n", lit->value);
    return true;
}

static bool compile_find_bound(compile_t *comp, const char *name, int *offset)
{
    for (env_t *env = comp->env; env; env = env->next) {
        if (!strcmp(name, env->name)) {
            *offset = env->offset;
            return true;
        }
    }
    return false;
}

static bool compile_emit_var(compile_t *comp, expr_var_t *var)
{
    int offset;
    if (!compile_find_bound(comp, var->name, &offset)) {
        printf("Unbound reference to '%s'\n", var->name);
        return false;
    }

    fprintf(comp->file, "\tmovq %d(%%rsp), %%rax\n", offset);
    return true;
}

static bool compile_emit_apply(compile_t *comp, expr_apply_t *app)
{
    if (!compile_emit_expr(comp, app->arg))
        return false;

    fputs("\tpushq %rax\n", comp->file);

    if (!compile_emit_expr(comp, app->fun))
        return false;

    fputs("\tcall *%rax\n"
          "\taddq $8, %rsp\n",
          comp->file);
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
            abort();
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
            return compile_lambdas(comp, lam->body)
                && compile_emit_lambda(comp, lam);
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
    if (!compile_lambdas(comp, expr))
        return false;

    fputs(".globl main\n"
          ".extern printf\n"
          "main:\n"
          "\tpushq %rbp\n"
          "\tmovq %rsp, %rbp\n",
          comp->file);

    if (!compile_emit_expr(comp, expr))
        return false;

    fputs("\tmovq %rax, %rsi\n"
          "\tleaq fmt(%rip), %rdi\n"
          "\tcall printf\n"
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
}
