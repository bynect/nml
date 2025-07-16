#ifndef NML_AST_H
#define NML_AST_H

#include <stdint.h>
#include <stddef.h>

#include "nml_str.h"
#include "nml_lex.h"
#include "nml_parse.h"
#include "nml_type.h"
#include "nml_const.h"
#include "nml_patn.h"
#include "nml_dbg.h"

typedef enum {
#define NODE(kind, node, _) AST_ ## kind,
#define EXPR
#include "nml_node.h"
#undef EXPR
#undef NODE
} Ast_Kind;

typedef struct Ast_Node {
	Ast_Kind kind;
	Type type;
	Location loc;
} Ast_Node;

typedef struct {
	Ast_Node node;
	Ast_Node *cond;
	Ast_Node *then;
	Ast_Node *m_else;
} Ast_If;

typedef struct {
	Ast_Node node;
	Ast_Node *value;
	struct Ast_Match_Arm {
		Patn_Node *patn;
		Ast_Node *expr;
	} *arms;
	size_t len;
} Ast_Match;

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
	Const_Value value;
} Ast_Const;

typedef struct Ast_Top {
	Ast_Node **items;
	size_t len;
} Ast_Top;

Ast_Node *ast_if_new(Arena *arena, Location loc, Ast_Node *cond, Ast_Node *then, Ast_Node *m_else);

Ast_Node *ast_match_new(Arena *arena, Location loc, Ast_Node *value, struct Ast_Match_Arm *arms, size_t len);

Ast_Node *ast_let_new(Arena *arena, Location loc, Sized_Str name, Type hint, Ast_Node *value, Ast_Node *expr);

Ast_Node *ast_fun_new(Arena *arena, Location loc, Sized_Str name, Type hint, Sized_Str *pars,
						size_t len, Ast_Node *body, Ast_Node *expr);

Ast_Node *ast_app_new(Arena *arena, Location loc, Ast_Node *fun, Ast_Node **args, size_t len);

Ast_Node *ast_ident_new(Arena *arena, Location loc, Sized_Str name);

Ast_Node *ast_unary_new(Arena *arena, Location loc, Token_Kind op, Ast_Node *lhs);

Ast_Node *ast_binary_new(Arena *arena, Location loc, Token_Kind op, Ast_Node *rhs, Ast_Node *lhs);

Ast_Node *ast_tuple_new(Arena *arena, Location loc, Ast_Node **items, size_t len);

Ast_Node *ast_const_new(Arena *arena, Location loc, Const_Value value);

#endif
