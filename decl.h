#ifndef DECL_H
#define DECL_H

#include "expr.h"

typedef enum {
    DECL_LET,
    //DECL_DATA,
    //DECL_TYPE,
} decl_tag_t;

typedef struct {
    decl_tag_t tag;
} decl_t;

typedef struct {
    decl_t base;
    type_scheme_t scheme;
    char *bound;
    expr_t *value;
    uint32_t id;
} decl_let_t;

decl_t *decl_let_new(char *bound, expr_t *value);

void decl_print(decl_t *decl);

void decl_println(decl_t *decl);

void decl_free(decl_t *decl);

#endif
