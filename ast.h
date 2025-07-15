#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    TYPE_VAR,
    TYPE_CON,
} type_tag_t;

typedef struct {
    type_tag_t tag;
} type_t;

typedef struct {
    type_t base;
    const char *name;
} type_var_t;

typedef struct {
    type_t base;
    const char *name;
    size_t n_args;
    type_t **args;
} type_con_t;

typedef enum {
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
    const char *name;
} expr_var_t;

typedef struct {
    expr_t base;
    const char *bound;
    expr_t *body;
} expr_lambda_t;

typedef struct {
    expr_t base;
    expr_t *fun;
    expr_t *arg;
} expr_apply_t;

typedef struct {
    expr_t base;
    const char *bound;
    expr_t *value;
    expr_t *body;
} expr_let_t;

type_t *type_var_new(const char *name);

type_t *type_con_new(const char *name, size_t n_args, type_t **args);

expr_t *expr_var_new(const char *name);

expr_t *expr_lambda_new(const char *bound, expr_t *body);

expr_t *expr_apply_new(expr_t *fun, expr_t *arg);

expr_t *expr_let_new(const char *bound, expr_t *value, expr_t *body);

#endif
