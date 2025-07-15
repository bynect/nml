#ifndef COMPILE_H
#define COMPILE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#include "ast.h"

typedef struct {
    FILE *file;
    int lam_id;
} compile_t;

void compile_init(compile_t *comp, FILE *file);

bool compile_expr(compile_t *comp, expr_t *expr);

void compile_free(compile_t *comp);

#endif
