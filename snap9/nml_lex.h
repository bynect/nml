#ifndef NML_LEX_H
#define NML_LEX_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "nml_str.h"
#include "nml_mac.h"
#include "nml_dbg.h"

typedef enum {
#define TOK(type, str, _, kind) TOK_ ## type,
#define SPEC
#define EXTRA
#define DELIM
#define OPER
#define LIT
#define KWD
#include "nml_tok.h"
#undef SPEC
#undef EXTRA
#undef DELIM
#undef OPER
#undef LIT
#undef KWD
#undef TOK
} Token_Kind;

typedef struct Token {
	Token_Kind kind;
	Location loc;
	Sized_Str str;
} Token;

typedef struct {
	Location loc;
	Sized_Str src;
	size_t soff;
	size_t off;
} Lexer;

void lexer_init(Lexer *lex, const Sized_Str src);

void lexer_reset(Lexer *lex);

MAC_HOT bool lexer_next(Lexer *lex, Token *tok);

#endif
