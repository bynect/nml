#ifndef PARSE_H
#define PARSE_H

#include "decl.h"
#include "lex.h"

typedef struct {
    lex_t lex;
    token_t next;
} parse_t;

void parse_init(parse_t *parse, const char *src, size_t len);

bool parse_eof(parse_t *parse);

bool parse_decl(parse_t *parse, decl_t **decl);

#endif
