#ifndef ENV_H
#define ENV_H

#include <sys/types.h>

typedef struct env {
    const char *name;
    long value;
    struct env *next;
} env_t;

env_t *env_append(env_t *tail, const char *name, long value);

env_t *env_update(env_t *tail, const char *name, long value);

env_t *env_remove(env_t *tail, const char *name);

ssize_t env_find(env_t *env, const char *name, long *value);

size_t env_length(env_t *env);

void env_print(env_t *env);

env_t *env_clear(env_t *head, env_t *until);

#endif
