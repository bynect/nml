#ifndef INFER_H
#define INFER_H

#include <stdbool.h>

#include "type.h"
#include "decl.h"
#include "expr.h"
#include "env.h"

typedef struct {
    long var_id;
    type_t *int_type;
    type_t *str_type;
    type_scheme_t ffi_extern_scheme;
    type_scheme_t ffi_call_scheme;
    env_t *env;
} infer_t;

void infer_init(infer_t *infer, env_t *env);

bool infer_expr(infer_t *infer, expr_t *expr);

bool infer_resolve(infer_t *infer, expr_t *expr);

bool infer_decl(infer_t *infer, decl_t *decl);

void infer_free(infer_t *infer);

#endif
