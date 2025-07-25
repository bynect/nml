#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"

env_t *env_append(env_t *tail, const char *name, intptr_t value)
{
    env_t *head = malloc(sizeof(env_t));
    head->name = name;
    head->next = tail;
    head->value = value;
    return head;
}

env_t *env_update(env_t *tail, const char *name, intptr_t value)
{
    for (env_t *env = tail; env; env = env->next) {
        if (!strcmp(name, env->name)) {
            env->value = value;
            return tail;
        }
    }
    return env_append(tail, name, value);
}

env_t *env_remove(env_t *tail, const char *name)
{
    for (env_t *env = tail, *prev = NULL; env; env = env->next) {
        if (!strcmp(name, env->name)) {
            if (prev == NULL) tail = env->next;
            else prev->next = env->next;
            free(env);
            break;
        }
        prev = env;
    }
    return tail;
}

ssize_t env_find(env_t *env, const char *name, intptr_t *value)
{
    size_t i = 0;
    for ( ; env; env = env->next) {
        if (!strcmp(name, env->name)) {
            if (value)
                *value = env->value;
            return i;
        }
        i++;
    }
    return -1;
}

size_t env_length(env_t *env)
{
    size_t i = 0;
    for ( ; env; env = env->next)
        i++;
    return i;
}

void env_print(env_t *env)
{
    for ( ; env; env = env->next) {
        printf("%s: %ld%s",
               env->name,
               env->value,
               env->next ? ", " : "");
    }
}

void env_println(env_t *env)
{
    env_print(env);
    puts("");
}

env_t *env_clear(env_t *head, env_t *until)
{
    while (head != until) {
        env_t *next = head->next;
        free(head);
        head = next;
    }
    return head;
}
