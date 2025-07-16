#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
	TOK_EOF,
	TOK_ERR,
	TOK_ARROW,
	TOK_PLUS,
	TOK_MINUS,
	TOK_STAR,
	TOK_SLASH,
	TOK_LPAR,
	TOK_RPAR,
	TOK_DOT,
	TOK_COMMA,
	TOK_EQ,
	TOK_GT,
	TOK_LT,
	TOK_GTE,
	TOK_LTE,
	TOK_AND,
	TOK_OR,
	TOK_UNIT,
	TOK_INT,
	TOK_FLOAT,
	TOK_STR,
	TOK_CHAR,
	TOK_IDENT,
	TOK_TRUE,
	TOK_FALSE,
	TOK_LET,
	TOK_FUN,
} Token_Type;

typedef struct {
	Token_Type type;
	uint16_t line;
	const char *src;
	size_t len;
} Token;

typedef struct {
	uint16_t line;
	const char *const src;
	size_t soff;
	size_t off;
	const size_t len;
} Lexer;

Lexer lexer_init(const char *const src, const size_t len);

bool lexer_next(Lexer *lex, Token *tok);

#endif
