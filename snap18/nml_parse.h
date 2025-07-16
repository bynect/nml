#ifndef NML_PARSE_H
#define NML_PARSE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nml_lex.h"
#include "nml_str.h"
#include "nml_err.h"
#include "nml_mem.h"
#include "nml_ast.h"

typedef enum {
	PARSE_OK = 0,
	PARSE_SYNC = 1 << 0,
	PARSE_ERR = 1 << 1,
	PARSE_MSG = 1 << 2,
} Parser_Flag;

typedef enum {
#define PREC(prec, _) PREC_ ## prec,
#include "nml_prec.h"
#undef PREC
} Parser_Prec;

typedef enum {
	ASSOC_RIGHT,
	ASSOC_LEFT,
	ASSOC_NONE,
} Parser_Assoc;

typedef struct {
	Lexer lex;
	Token next;
	Token curr;
	Token prev;
	Parser_Flag flags;
	Arena *arena;
	Error_List *err;
	Parser_Prec prec;
} Parser;

void parser_init(Parser *parse, Arena *arena, const Source *src);

MAC_HOT bool parser_parse(Parser *parse, Ast_Top *ast, Error_List *err);

#endif
