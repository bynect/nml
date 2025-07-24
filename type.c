#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include "type.h"

type_t *type_var_new(type_id_t id)
{
    type_var_t *type = calloc(1, sizeof(type_var_t));
    type->base.tag = TYPE_VAR;
    type->id = id;
    return (type_t *)type;
}

type_t *type_con_new(const char *name, size_t n_args, type_t **args)
{
    type_con_t *type = calloc(1, sizeof(type_con_t));
    type->base.tag = TYPE_CON;
    type->name = name;
    type->n_args = n_args;
    type->args = args;
    return (type_t *)type;
}

type_t *type_con_new_v(const char *name, size_t n_args, ...)
{
    va_list vargs;
    va_start(vargs, n_args);

    type_t **args = calloc(n_args, sizeof(type_t *));
    for (size_t i = 0; i < n_args; i++)
        args[i] = va_arg(vargs, type_t *);

    va_end(vargs);
    return type_con_new(name, n_args, args);
}

void type_print(type_t *type)
{
    switch (type->tag) {
        case TYPE_VAR: {
            type_var_t *var = (type_var_t *)type;
            printf("'t%u", var->id);
            break;
        }

        case TYPE_CON: {
            type_con_t *con = (type_con_t *)type;
            bool infix = !strcmp(con->name, "->");

            if (!infix)
                fprintf(stdout, "%s ", con->name);

            if (con->n_args > 0) {

                for (size_t i = 0; i < con->n_args; i++) {
                    bool paren = con->args[i]->tag != TYPE_VAR &&
                        !(con->args[i]->tag == TYPE_CON &&
                        (((type_con_t *)con->args[i])->n_args == 0
                        || !strcmp(((type_con_t *)con->args[i])->name, "->")));

                    if (paren) putc('(', stdout);
                    type_print(con->args[i]);
                    if (paren) putc(')', stdout);


                    if (i != con->n_args - 1) {
                        if (infix) fprintf(stdout, " %s ", con->name);
                        else putc(' ', stdout);
                    }
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

void type_scheme_init(type_scheme_t *scheme, type_t *type, size_t n_vars, type_id_t *vars)
{
    scheme->type = type;
    scheme->n_vars = n_vars;
    scheme->vars = vars;
}

bool type_scheme_instantiate(type_scheme_t *scheme, type_var_t **new, type_t **out)
{
    if (scheme->type->tag == TYPE_VAR) {
        type_var_t *var = (type_var_t *)scheme->type;
        type_t *res = scheme->type;

        for (size_t i = 0; i < scheme->n_vars; i++) {
            if (scheme->vars[i] == var->id) {
                res = (type_t *)new[i];
                break;
            }
        }

        *out = res;
        return true;
    }

    if (scheme->type->tag == TYPE_CON) {
        type_con_t *con = (type_con_t *)scheme->type;
        type_t **args = calloc(con->n_args, sizeof(type_t *));

        type_scheme_t copy;
        memcpy(&copy, scheme, sizeof(type_scheme_t));

        for (size_t i = 0; i < con->n_args; i++) {
            copy.type = con->args[i];
            if (!type_scheme_instantiate(&copy, new, &args[i]))
                return false;
        }

        *out = type_con_new(con->name, con->n_args, args);
        return true;
    }

    return false;
}

void type_scheme_print(type_scheme_t *scheme)
{
    if (scheme->n_vars > 0) {
        fputs("forall ", stdout);

        for (size_t i = 0; i < scheme->n_vars; i++) {
            printf("'t%u ", scheme->vars[i]);
        }

        fputs(". ", stdout);
    }

    type_print(scheme->type);
}

void type_scheme_println(type_scheme_t *scheme)
{
    type_scheme_print(scheme);
    puts("");
}
