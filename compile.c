#include <stdio.h>
#include <stdlib.h>

#include "compile.h"
#include "ast.h"

void compile_init(compile_t *comp, FILE *file)
{
    comp->file = file;
    comp->lam_id = 0;
}

static bool compile_emit_expr(compile_t *comp, expr_t *expr);

static bool compile_emit_lambda(compile_t *comp, expr_lambda_t *lam)
{
    if (lam->id != NULL) {
        fprintf(comp->file, "\tmovq %s, %%rax\n", lam->id);
        return true;
    }

    char *id = malloc(16);
    snprintf(id, 16, "lambda_%d", comp->lam_id++);
    lam->id = id;

    fprintf(comp->file, "%s:\n", id);

    compile_emit_expr(comp, lam->body);

    fprintf(comp->file, "ret\n");

    return true;
}

static bool compile_emit_lit(compile_t *comp, expr_lit_t *lit)
{
    fprintf(comp->file, "\tmovq $%ld, %%rax\n", lit->value);
    return true;
}

static bool compile_emit_var(compile_t *comp, expr_var_t *var)
{
    // TODO
    return false;
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
            abort();
            //return compile_lambdas(comp, lam->body)
            //    && compile_emit_lambda(comp, lam);
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            abort();
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

    compile_emit_expr(comp, expr);

    fputs("\tmovq %rax, %rsi\n"
          "\tleaq fmt(%rip), %rdi\n"
          "\tcall printf\n"
          "\tleave\n"
          "\tret\n"
          ".section .rodata\n"
          "fmt: .asciz \"Result: %ld\\n\"\n"
          ".section .note.GNU-stack,\"\",@progbits\n",
          comp->file);

    return true;
}

void compile_free(compile_t *comp)
{
}
