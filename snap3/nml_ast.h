#ifndef NML_AST_H
#define NML_AST_H

#include <stdint.h>
#include <stddef.h>

#include "nml_str.h"
#include "nml_lex.h"
#include "nml_parse.h"
#include "nml_type.h"

typedef enum {
#define NODE(kind, _) AST_ ## kind,
#include "nml_node.h"
#undef NODE
} Ast_Kind;

typedef enum {
	CONST_UNIT,
	CONST_INT,
	CONST_FLOAT,
	CONST_STR,
	CONST_CHAR,
	CONST_BOOL,
} Const_Kind;

typedef struct Ast_Node {
	Ast_Kind kind;
	Type type;
} Ast_Node;

typedef struct {
	Ast_Node node;
	Ast_Node *cond;
	Ast_Node *then;
	Ast_Node *m_else;
} Ast_If;

typedef struct {
	Ast_Node node;
	Sized_Str name;
	Type bind;
	Type hint;
	Ast_Node *value;
	Ast_Node *expr;
} Ast_Let;

typedef struct {
	Ast_Node node;
	Type fun;
	Type hint;
	Type rec;
	Type ret;
	Sized_Str name;
	Sized_Str *pars;
	size_t len;
	Ast_Node *body;
	Ast_Node *expr;
} Ast_Fun;

typedef struct {
	Ast_Node node;
	Ast_Node *fun;
	Ast_Node **args;
	size_t len;
} Ast_App;

typedef struct {
	Ast_Node node;
	Sized_Str name;
} Ast_Ident;

typedef struct {
	Ast_Node node;
	Token_Kind op;
	Ast_Node *lhs;
} Ast_Unary;

typedef struct {
	Ast_Node node;
	Token_Kind op;
	Ast_Node *rhs;
	Ast_Node *lhs;
} Ast_Binary;

typedef struct {
	Ast_Node node;
	Ast_Node **items;
	size_t len;
} Ast_Tuple;

typedef struct {
	Ast_Node node;
	Const_Kind kind;
	union {
		int32_t c_int;
		double c_float;
		Sized_Str c_str;
		char c_char;
		bool c_bool;
	} value;
} Ast_Const;

typedef struct Ast_Top {
	Ast_Node **items;
	size_t len;
} Ast_Top;

Ast_Node *ast_if_new(Arena *arena, Ast_Node *cond, Ast_Node *then, Ast_Node *m_else);

Ast_Node *ast_let_new(Arena *arena, Sized_Str name, Type hint, Ast_Node *value, Ast_Node *expr);

Ast_Node *ast_fun_new(Arena *arena, Sized_Str name, Type hint, Sized_Str *pars,
						size_t len, Ast_Node *body, Ast_Node *expr);

Ast_Node *ast_app_new(Arena *arena, Ast_Node *fun, Ast_Node **args, size_t len);

Ast_Node *ast_ident_new(Arena *arena, const Sized_Str name);

Ast_Node *ast_unary_new(Arena *arena, Token_Kind op, Ast_Node *lhs);

Ast_Node *ast_binary_new(Arena *arena, Token_Kind op, Ast_Node *rhs, Ast_Node *lhs);

Ast_Node *ast_tuple_new(Arena *arena, Ast_Node **items, size_t len);

Ast_Node *ast_const_new(Arena *arena, Const_Kind kind, const void *value);

#endif
