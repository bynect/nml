#include "decl.h"
#include <stdio.h>
#include <stdlib.h>

decl_t *decl_let_new(char *bound, expr_t *value)
{
    decl_let_t *decl = calloc(1, sizeof(decl_let_t));
    decl->base.tag = DECL_LET;
    decl->bound = bound;
    decl->value = value;
    return (decl_t *)decl;
}

void decl_print(decl_t *decl)
{
    if (decl->tag == DECL_LET) {
        decl_let_t *let = (decl_let_t *)decl;
        printf("let %s", let->bound);

        if (let->scheme.type) {
            fputs(" : ", stdout);
            type_scheme_print(&let->scheme);
        }

        fputs(" = ", stdout);
        expr_print(let->value);
        putc(';', stdout);
    }
}

void decl_println(decl_t *decl)
{
    decl_print(decl);
    puts("");
}

void decl_free(decl_t *decl)
{
    if (decl->tag == DECL_LET) {
        decl_let_t *let = (decl_let_t *)decl;
        expr_free(let->value);
        free(let->bound);
    }

    free(decl);
}
