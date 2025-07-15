#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ast.h"

type_t *type_var_new(const char *name)
{
    type_var_t *type = malloc(sizeof(type_var_t));
    type->base.tag = TYPE_VAR;
    type->name = name;
    return (type_t *)type;
}

type_t *type_con_new(const char *name, size_t n_args, type_t **args)
{
    type_con_t *type = malloc(sizeof(type_con_t));
    type->base.tag = TYPE_CON;
    type->name = name;
    type->n_args = n_args;
    type->args = args;
    return (type_t *)type;
}

void type_print(type_t *type)
{
    switch (type->tag) {
        case TYPE_VAR: {
            type_var_t *var = (type_var_t *)type;
            printf("'%s", var->name);
            break;
        }

        case TYPE_CON: {
            type_con_t *con = (type_con_t *)type;
            fputs(con->name, stdout);

            if (con->n_args > 0) {
                for (size_t i = 0; i < con->n_args; i++) {
                    bool paren = con->args[i]->tag != TYPE_VAR &&
                        !(con->args[i]->tag == TYPE_CON &&
                        ((type_con_t *)con->args[i])->n_args == 0);

                    if (paren) putc('(', stdout);
                    type_print(con->args[i]);
                    if (paren) putc(')', stdout);
                    putc(' ', stdout);
                }
            }
            break;
        }
    }
}

void type_println(type_t *type)
{
    type_print(type);
    puts("");
}

void type_free(type_t *type)
{
    if (type->tag == TYPE_CON) {
        type_con_t *con = (type_con_t *)type;
        for (size_t i = 0; i < con->n_args; i++)
            type_free(con->args[i]);
        free(con->args);
    }

    free(type);
}

expr_t *expr_var_new(const char *name)
{
    expr_var_t *expr = malloc(sizeof(expr_var_t));
    expr->base.tag = EXPR_VAR;
    expr->name = name;
    return (expr_t *)expr;
}

expr_t *expr_lambda_new(const char *bound, expr_t *body)
{
    expr_lambda_t *expr = malloc(sizeof(expr_lambda_t));
    expr->base.tag = EXPR_LAMBDA;
    expr->bound = bound;
    expr->body = body;
    return (expr_t *)expr;
}

expr_t *expr_apply_new(expr_t *fun, expr_t *arg)
{
    expr_apply_t *expr = malloc(sizeof(expr_apply_t));
    expr->base.tag = EXPR_APPLY;
    expr->fun = fun;
    expr->arg = arg;
    return (expr_t *)expr;
}

expr_t *expr_let_new(const char *bound, expr_t *value, expr_t *body)
{
    expr_let_t *expr = malloc(sizeof(expr_let_t));
    expr->base.tag = EXPR_LET;
    expr->bound = bound;
    expr->value = value;
    expr->body = body;
    return (expr_t *)expr;
}

void expr_print(expr_t *expr)
{
    switch (expr->tag) {
        case EXPR_VAR: {
            expr_var_t *var = (expr_var_t *)expr;
            fputs(var->name, stdout);
            break;
        }

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            printf("%s -> ", lam->bound);
            expr_print(lam->body);
            break;
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;

            bool fun_paren = app->fun->tag != EXPR_VAR;
            if (fun_paren) putc('(', stdout);
            expr_print(app->fun);
            if (fun_paren) putc(')', stdout);
            putc(' ', stdout);

            bool arg_paren = app->arg->tag != EXPR_VAR;
            if (arg_paren) putc('(', stdout);
            expr_print(app->arg);
            if (arg_paren) putc(')', stdout);
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
    free(expr);
}

