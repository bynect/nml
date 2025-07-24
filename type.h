#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    TYPE_VAR,
    TYPE_CON,
} type_tag_t;

typedef uint32_t type_id_t;

typedef struct {
    type_tag_t tag;
} type_t;

typedef struct {
    type_t base;
    type_id_t id;
    type_t *forward;
} type_var_t;

typedef struct {
    type_t base;
    const char *name;
    size_t n_args;
    type_t **args;
} type_con_t;

typedef struct {
    size_t n_vars;
    type_id_t *vars;
    type_t *type;
} type_scheme_t;


type_t *type_var_new(type_id_t id);

type_t *type_con_new(const char *name, size_t n_args, type_t **args);

type_t *type_con_new_v(const char *name, size_t n_args, ...);

void type_print(type_t *type);

void type_println(type_t *type);

void type_free(type_t *type);

void type_scheme_init(type_scheme_t *scheme, type_t *type, size_t n_vars, type_id_t *vars);

bool type_scheme_instantiate(type_scheme_t *scheme, type_var_t **new, type_t **out);

void type_scheme_print(type_scheme_t *scheme);

void type_scheme_println(type_scheme_t *scheme);

#endif
