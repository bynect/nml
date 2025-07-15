#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "expr.h"
#include "env.h"

expr_t *expr_lit_new(int64_t value)
{
    expr_lit_t *expr = calloc(1, sizeof(expr_lit_t));
    expr->base.tag = EXPR_LIT;
    expr->value = value;
    return (expr_t *)expr;
}

expr_t *expr_var_new(const char *name)
{
    expr_var_t *expr = calloc(1, sizeof(expr_var_t));
    expr->base.tag = EXPR_VAR;
    expr->name = name;
    return (expr_t *)expr;
}

expr_t *expr_lambda_new(const char *bound, expr_t *body)
{
    expr_lambda_t *expr = calloc(1, sizeof(expr_lambda_t));
    expr->base.tag = EXPR_LAMBDA;
    expr->bound = bound;
    expr->body = body;
    return (expr_t *)expr;
}

expr_t *expr_apply_new(expr_t *fun, expr_t *arg)
{
    expr_apply_t *expr = calloc(1, sizeof(expr_apply_t));
    expr->base.tag = EXPR_APPLY;
    expr->fun = fun;
    expr->arg = arg;
    return (expr_t *)expr;
}

expr_t *expr_let_new(const char *bound, expr_t *value, expr_t *body)
{
    expr_let_t *expr = calloc(1, sizeof(expr_let_t));
    expr->base.tag = EXPR_LET;
    expr->bound = bound;
    expr->value = value;
    expr->body = body;
    return (expr_t *)expr;
}

expr_t *expr_annotate(expr_t *expr, type_t *type)
{
    if (expr->type != NULL)
        type_free(expr->type);

    expr->type = type;
    return expr;
}

void expr_print(expr_t *expr)
{
    switch (expr->tag) {
        case EXPR_LIT: {
            expr_lit_t *lit = (expr_lit_t *)expr;
            printf("%ld", lit->value);
            break;
        }

        case EXPR_VAR: {
            expr_var_t *var = (expr_var_t *)expr;
            fputs(var->name, stdout);
            break;
        }

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            fputs(lam->bound, stdout);

            if (expr->type && expr->type->tag == TYPE_CON
                && !strcmp(((type_con_t *)expr->type)->name, "->")) {
                fputs(" : ", stdout);
                type_print(((type_con_t *)expr->type)->args[0]);
            }

            fputs(" -> ", stdout);
            expr_print(lam->body);
            break;
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            bool fun_paren = app->fun->tag != EXPR_LIT && app->fun->tag != EXPR_VAR;
            bool arg_paren = app->arg->tag != EXPR_LIT && app->arg->tag != EXPR_VAR;

            if (fun_paren) putc('(', stdout);
            expr_print(app->fun);
            if (fun_paren) putc(')', stdout);
            putc(' ', stdout);

            if (arg_paren) putc('(', stdout);
            expr_print(app->arg);
            if (arg_paren) putc(')', stdout);
            break;
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            printf("let %s", let->bound);

            if (let->value->type) {
                fputs(" : ", stdout);
                type_print(let->value->type);
            }

            fputs(" = ", stdout);
            expr_print(let->value);

            printf(" in ");
            expr_print(let->body);
            break;
        }
    }
}

void expr_println(expr_t *expr)
{
    expr_print(expr);
    puts("");
}

void expr_free(expr_t *expr)
{
    switch (expr->tag) {
        case EXPR_LIT:
        case EXPR_VAR:
            break;

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            expr_free(lam->body);
            free(lam->id);
            env_clear(lam->freevars, NULL);
            break;
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            expr_free(app->fun);
            expr_free(app->arg);
            break;
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            expr_free(let->value);
            expr_free(let->body);
            break;
        }
    }
    free(expr);
}
