#ifndef LEX_H
#define LEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TOK_EOF,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_LET,
    TOK_IN,
    TOK_EQ,
    TOK_EQEQ,
    TOK_ARROW,
    TOK_SEMI,
    TOK_BACK,
    TOK_LPAR,
    TOK_RPAR,
    TOK_LBRACK,
    TOK_RBRACK,
    TOK_COL,
    TOK_COLCOL,
    TOK_ERROR,
} token_type_t;

typedef struct {
    token_type_t type;
    uint32_t line;
    const char *str;
    size_t len;
} token_t;

typedef struct {
    uint32_t line;
    const char *src;
    size_t len;
    size_t off;
    size_t tok_off;
    uint32_t tok_line;
} lex_t;

void lex_init(lex_t *lex, const char *src, size_t len);

void lex_next(lex_t *lex, token_t *next);

extern const char *tokens[TOK_ERROR + 1];

#endif
