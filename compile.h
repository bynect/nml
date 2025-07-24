#ifndef COMPILE_H
#define COMPILE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "decl.h"
#include "expr.h"
#include "env.h"

typedef struct {
    FILE *file;
    uint32_t lambda_id;
    uint32_t init_id;
    long let_n;
    env_t *env;
    decl_let_t *main;
	size_t n_strings;
	const char **strings;
} compile_t;

void compile_init(compile_t *comp, FILE *file);

bool compile_decl(compile_t *comp, decl_t *decl);

bool compile_main(compile_t *comp);

void compile_free(compile_t *comp);

#endif
