#ifndef COMPILE_H
#define COMPILE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "expr.h"
#include "env.h"

typedef struct {
    FILE *file;
    long lam_id;
    long let_n;
    env_t *env;
} compile_t;

void compile_init(compile_t *comp, FILE *file);

bool compile_expr(compile_t *comp, expr_t *expr);

void compile_free(compile_t *comp);

#endif
