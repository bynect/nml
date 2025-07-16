#ifndef NML_AST_H
#define NML_AST_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mem.h"
#include "nml_str.h"
#include "nml_lex.h"
#include "nml_parse.h"
#include "nml_type.h"

typedef enum {
#define TAG(name, _, kind)	CONST_ ## name,
#define TYPE
#define BASE
#include "nml_tag.h"
#undef BASE
#undef TYPE
#undef TAG
} Const_Kind;

typedef enum {
	AST_EXPR,
	AST_PATN,
} Ast_Kind;

typedef enum {
#define NODE(name, _, kind) EXPR_ ## name,
#define EXPR
#include "nml_node.h"
#undef EXPR
#undef NODE
} Expr_Kind;

typedef enum {
#define NODE(name, _, kind) PATN_ ## name,
#define PATN
#include "nml_node.h"
#undef PATN
#undef NODE
} Patn_Kind;

typedef struct {
	Const_Kind kind;
	union {
		int32_t c_int;
		double c_float;
		Sized_Str c_str;
		char c_char;
		bool c_bool;
	} value;
} Const_Node;

typedef struct Ast_Node {
	Ast_Kind kind;
} Ast_Node;

typedef struct Expr_Node {
	Ast_Node node;
	Expr_Kind kind;
	Type type;
} Expr_Node;

typedef struct Patn_Node {
	Ast_Node node;
	Patn_Kind kind;
	Type type;
} Patn_Node;

typedef struct {
	Expr_Node node;
	Expr_Node *cond;
	Expr_Node *then;
	Expr_Node *m_else;
} Expr_If;

typedef struct {
	Expr_Node *patt;
	Expr_Node *expr;
} Match_Arm;

typedef struct {
	Expr_Node node;
	Expr_Node *value;
	Match_Arm *arms;
	size_t len;
} Expr_Match;

typedef struct {
	Expr_Node node;
	Sized_Str name;
	Type bind;
	Type hint;
	Expr_Node *value;
	Expr_Node *expr;
} Expr_Let;

typedef struct {
	Expr_Node node;
	Type fun;
	Type rec;
	Type ret;
	Sized_Str name;
	Type hint;
	Sized_Str *pars;
	size_t len;
	Expr_Node *body;
	Expr_Node *expr;
} Expr_Fun;

typedef struct {
	Expr_Node node;
	Expr_Node *fun;
	Expr_Node **args;
	size_t len;
} Expr_App;

typedef struct {
	Expr_Node node;
	Sized_Str name;
} Expr_Ident;

typedef struct {
	Expr_Node node;
	Token_Kind op;
	Expr_Node *lhs;
} Expr_Unary;

typedef struct {
	Expr_Node node;
	Token_Kind op;
	Expr_Node *rhs;
	Expr_Node *lhs;
} Expr_Binary;

typedef struct {
	Expr_Node node;
	Expr_Node **items;
	size_t len;
} Expr_Tuple;

typedef struct {
	Expr_Node node;
	Const_Node cons;
} Expr_Const;

typedef struct {
	Patn_Node node;
	Sized_Str name;
} Patn_Ident;

typedef struct {
	Patn_Node node;
	Patn_Node **items;
	size_t len;
} Patn_Tuple;

typedef struct {
	Patn_Node node;
	Const_Node cons;
} Patn_Const;

typedef struct Ast_Module {
	Expr_Node **items;
	size_t len;
} Ast_Module;

Expr_Node *expr_if_new(Arena *arena, Expr_Node *cond, Expr_Node *then, Expr_Node *m_else);

Expr_Node *expr_match_new(Arena *arena, Expr_Node *value, Match_Arm *arms, size_t len);

Expr_Node *expr_let_new(Arena *arena, Sized_Str name, Type hint, Expr_Node *value, Expr_Node *expr);

Expr_Node *expr_fun_new(Arena *arena, Sized_Str name, Type hint, Sized_Str *pars,
						size_t len, Expr_Node *body, Expr_Node *expr);

Expr_Node *expr_app_new(Arena *arena, Expr_Node *fun, Expr_Node **args, size_t len);

Expr_Node *expr_ident_new(Arena *arena, const Sized_Str name);

Expr_Node *expr_unary_new(Arena *arena, Token_Kind op, Expr_Node *lhs);

Expr_Node *expr_binary_new(Arena *arena, Token_Kind op, Expr_Node *rhs, Expr_Node *lhs);

Expr_Node *expr_tuple_new(Arena *arena, Expr_Node **items, size_t len);

Expr_Node *expr_const_new(Arena *arena, Const_Kind kind, const void *value);

#endif
