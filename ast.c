#include <stdio.h>
#include <stdlib.h>

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
