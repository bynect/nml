#ifndef EXPR_H
#define EXPR_H

#include "type.h"

typedef enum {
    EXPR_LIT,
    EXPR_VAR,
    EXPR_LAMBDA,
    EXPR_APPLY,
    EXPR_LET,
} expr_tag_t;

typedef struct {
    expr_tag_t tag;
    type_t *type;
} expr_t;

typedef struct {
    expr_t base;
    bool is_str;
    union {
        int64_t intv;
        char *strv;
    };
} expr_lit_t;

typedef struct {
    expr_t base;
    char *name;
} expr_var_t;

typedef struct {
    expr_t base;
    char *bound;
    expr_t *body;
    char *id;
    struct env *freevars;
} expr_lambda_t;

typedef struct {
    expr_t base;
    expr_t *fun;
    expr_t *arg;
} expr_apply_t;

typedef struct {
    expr_t base;
    char *bound;
    expr_t *value;
    expr_t *body;
    type_scheme_t scheme;
} expr_let_t;

expr_t *expr_lit_new_int(int64_t intv);

expr_t *expr_lit_new_str(char *strv);

expr_t *expr_var_new(char *name);

expr_t *expr_lambda_new(char *bound, expr_t *body);

expr_t *expr_apply_new(expr_t *fun, expr_t *arg);

expr_t *expr_let_new(char *bound, expr_t *value, expr_t *body);

expr_t *expr_annotate(expr_t *expr, type_t *type);

void expr_print(expr_t *expr);

void expr_println(expr_t *expr);

void expr_free(expr_t *expr);

#endif
