#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "infer.h"
#include "decl.h"
#include "env.h"
#include "type.h"

void infer_init(infer_t *infer, env_t *env)
{
    infer->var_id = 0;
    infer->int_type = type_con_new("int", 0, NULL);
    infer->env = env;
}

static type_t *infer_freshvar(infer_t *infer)
{
    return type_var_new(infer->var_id++);
}

static bool infer_type_find(type_t *type, type_t **resolve)
{
    if (type == NULL)
        return false;

    do {
        *resolve = type;

        if (type->tag != TYPE_VAR) break;
        type = ((type_var_t *)type)->forward;
    } while (type);

    return true;
}

static bool infer_type_resolve(type_t *type, type_t **resolve)
{
    type_t *res;
    if (!infer_type_find(type, &res))
        return false;

    *resolve = res;
    if (res->tag == TYPE_CON) {
        type_con_t *con = (type_con_t *)res;

        for (size_t i = 0; i < con->n_args; i++) {
            if (!infer_type_resolve(con->args[i], &res))
                return false;

            con->args[i] = res;
        }
    }
    return true;
}

static bool infer_make_equal_to(type_t *type, type_t *other)
{
    type_t *res;
    if (!infer_type_find(type, &res))
        return false;

    if (res->tag != TYPE_VAR) {
        printf("Type already resolved to ");
        type_println(res);
        return false;
    }

    ((type_var_t *)res)->forward = other;
    return true;
}

static bool infer_type_occurs(type_t *t1, type_t *t2)
{
    type_t *res;
    if (!infer_type_find(t2, &res))
        return false;

    if (res->tag == TYPE_VAR
        && ((type_var_t *)res)->id == ((type_var_t *)t1)->id)
        return true;

    if (res->tag == TYPE_CON) {
        type_con_t *con = (type_con_t *)res;
        for (size_t i = 0; i < con->n_args; i++) {
            if (infer_type_occurs(t1, con->args[i]))
                return true;
        }
    }

    return false;
}

static bool infer_type_unify(type_t *t1, type_t *t2)
{
    type_t *t1_res, *t2_res;
    if (!infer_type_find(t1, &t1_res) || !infer_type_find(t2, &t2_res))
        return false;

    if (t1_res->tag == TYPE_VAR) {
        if (infer_type_occurs(t1_res, t2_res)) {
            printf("Occurs check failed for ");
            type_println(t1_res);
            return false;
        }
        return infer_make_equal_to(t1_res, t2_res);
    }

    if (t2_res->tag == TYPE_VAR) {
        if (infer_type_occurs(t2_res, t1_res)) {
            printf("Occurs check failed for ");
            type_println(t2_res);
            return false;
        }
        return infer_make_equal_to(t2_res, t1_res);
    }

    if (t1_res->tag == TYPE_CON && t2_res->tag == TYPE_CON) {
        type_con_t *t1_con = (type_con_t *)t1_res;
        type_con_t *t2_con = (type_con_t *)t2_res;

        if (strcmp(t1_con->name, t2_con->name) || t1_con->n_args != t2_con->n_args) {
            printf("Failed to unify ");
            type_print(t1_res);
            printf(" and ");
            type_println(t2_res);
            return false;
        }

        for (size_t i = 0; i < t1_con->n_args; i++) {
            if (!infer_type_unify(t1_con->args[i], t2_con->args[i]))
                return false;
        }
        return true;
    }

    printf("Invalid type unification\n");
    return false;
}

static bool infer_instantiate(infer_t *infer, type_scheme_t *scheme, type_t **type)
{
    type_var_t **vars = NULL;

    if (scheme->n_vars > 0) {
        vars = calloc(scheme->n_vars, sizeof(type_var_t *));
        for (size_t i = 0; i < scheme->n_vars; i++)
            vars[i] = (type_var_t *)infer_freshvar(infer);
    }

    bool ok = type_scheme_instantiate(scheme, vars, type);
    free(vars);
    return ok;
}

// FIXME: This is very inefficient and probably incorrect
static void infer_collect(infer_t *infer, type_t *type, size_t *n_vars, type_id_t **vars)
{
    if (type->tag == TYPE_VAR) {
        type_var_t *var = (type_var_t *)type;

        for (env_t *env = infer->env; env; env = env->next) {
            type_scheme_t *scheme = (type_scheme_t *)env->value;

            // If the var is quantified, skip this scheme
            for (size_t i = 0; i < scheme->n_vars; i++) {
                if (var->id == scheme->vars[i])
                    goto next;
            }

            // If the var is in the scheme's type, stop
            if (infer_type_occurs(type, scheme->type))
                return;
next:;
        }

        for (size_t i = 0; i < *n_vars; i++) {
            if (var->id == (*vars)[i])
                return;
        }

        *vars = realloc(*vars, ++(*n_vars) * sizeof(type_id_t));
        (*vars)[*n_vars - 1] = var->id;
        return;
    }

    if (type->tag == TYPE_CON) {
        type_con_t *con = (type_con_t *)type;
        for (size_t i = 0; i < con->n_args; i++)
            infer_collect(infer, con->args[i], n_vars, vars);
    }
}

static bool infer_generalize(infer_t *infer, type_scheme_t *scheme, type_t *type)
{
    type_t *res;
    if (!infer_type_resolve(type, &res))
        return false;

    size_t n_vars = 0;
    type_id_t *vars = NULL;
    infer_collect(infer, res, &n_vars, &vars);
    type_scheme_init(scheme, res, n_vars, vars);
    return true;
}

bool infer_expr(infer_t *infer, expr_t *expr)
{
    switch (expr->tag) {
        case EXPR_LIT:
            expr->type = infer->int_type;
            return true;

        case EXPR_VAR: {
            expr_var_t *var = (expr_var_t *)expr;
            expr->type = infer_freshvar(infer);

            type_scheme_t *scheme;
            if (env_find(infer->env, var->name, (intptr_t *)&scheme) < 0) {
                printf("Unbound reference to '%s'\n", var->name);
                return false;
            }

            type_t *inst;
            if (!infer_instantiate(infer, scheme, &inst)) {
                printf("Failed to instantiate ");
                type_scheme_println(scheme);
                return false;
            }

            return infer_type_unify(expr->type, inst);
        }

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            expr->type = infer_freshvar(infer);
            type_t *fresh = infer_freshvar(infer);

            type_scheme_t scheme;
            type_scheme_init(&scheme, fresh, 0, NULL);

            env_t *env = infer->env;
            infer->env = env_append(env, lam->bound, (intptr_t)&scheme);

            if (!infer_expr(infer, lam->body)) {
                printf("Failed to infer lambda body\n");
                return false;
            }

            type_t **args = malloc(sizeof(type_t *) * 2);
            args[0] = fresh;
            args[1] = lam->body->type;

            infer->env = env_clear(infer->env, env);
            return infer_type_unify(expr->type, type_con_new("->", 2, args));
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            expr->type = infer_freshvar(infer);

            if (!infer_expr(infer, app->fun)) {
                printf("Failed to infer apply function\n");
                return false;
            }

            if (!infer_expr(infer, app->arg)) {
                printf("Failed to infer apply argument\n");
                return false;
            }

            type_t **args = malloc(sizeof(type_t *) * 2);
            args[0] = app->arg->type;
            args[1] = expr->type;

            type_t *arrow = type_con_new("->", 2, args);
            return infer_type_unify(app->fun->type, arrow);
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            expr->type = infer_freshvar(infer);

            if (!infer_expr(infer, let->value)) {
                printf("Failed to infer let value\n");
                return false;
            }

            if (!infer_generalize(infer, &let->scheme, let->value->type)) {
                printf("Failed to generalize let\n");
                return false;
            }

            env_t *env = infer->env;
            infer->env = env_append(env, let->bound, (intptr_t)&let->scheme);
            if (!infer_expr(infer, let->body)) {
                printf("Failed to infer let body\n");
                return false;
            }

            infer->env = env_clear(infer->env, env);
            return infer_type_unify(expr->type, let->body->type);
        }
    }

    return false;
}

bool infer_resolve(infer_t *infer, expr_t *expr)
{
    type_t *res;
    if (!infer_type_resolve(expr->type, &res))
        return false;

    expr->type = res;
    switch (expr->tag) {
        case EXPR_LIT:
        case EXPR_VAR:
            return true;

        case EXPR_LAMBDA: {
            expr_lambda_t *lam = (expr_lambda_t *)expr;
            return infer_resolve(infer, lam->body);
        }

        case EXPR_APPLY: {
            expr_apply_t *app = (expr_apply_t *)expr;
            return infer_resolve(infer, app->fun)
                && infer_resolve(infer, app->arg);
        }

        case EXPR_LET: {
            expr_let_t *let = (expr_let_t *)expr;
            return infer_resolve(infer, let->value)
                && infer_resolve(infer, let->body);
        }
    }
    return false;
}

bool infer_decl(infer_t *infer, decl_t *decl)
{
    switch (decl->tag) {
        case DECL_LET: {
            decl_let_t *let = (decl_let_t *)decl;

            if (!infer_expr(infer, let->value)) {
                printf("Failed to infer let value\n");
                return false;
            }

            if (!infer_resolve(infer, let->value)) {
                printf("Failed to resolve let type\n");
                return false;
            }

            if (!infer_generalize(infer, &let->scheme, let->value->type)) {
                printf("Failed to generalize let type\n");
                return false;
            }

            infer->env = env_append(infer->env, let->bound, (intptr_t)&let->scheme);
            return true;
        }
    }
    return false;
}

void infer_free(infer_t *infer)
{
    // TODO
}
