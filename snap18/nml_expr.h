#ifndef NML_EXPR_H
#define NML_EXPR_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_str.h"
#include "nml_lex.h"
#include "nml_type.h"
#include "nml_const.h"
#include "nml_patn.h"
#include "nml_dbg.h"
#include "nml_tab.h"

typedef enum {
#define NODE(kind, node, _) EXPR_ ## kind,
#define EXPR
#include "nml_node.h"
#undef EXPR
#undef NODE
} Expr_Kind;

typedef struct Expr_Node {
	Expr_Kind kind;
	Type type;
	Location loc;
} Expr_Node;

typedef struct {
	Expr_Node node;
	Expr_Node *cond;
	Expr_Node *b_then;
	Expr_Node *b_else;
} Expr_If;

typedef struct {
	Expr_Node node;
	Expr_Node *value;
	struct Expr_Match_Arm {
		Patn_Node *patn;
		Expr_Node *expr;
	} *arms;
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
	Type hint;
	Type rec;
	Type ret;
	Sized_Str name;
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
	Const_Value value;
} Expr_Const;

TAB_DECL(expr_tab, Expr, Sized_Str, Expr_Node *)

MAC_HOT Expr_Node *expr_if_new(Arena *arena, Location loc, Expr_Node *cond, Expr_Node *b_then, Expr_Node *b_else);

MAC_HOT Expr_Node *expr_match_new(Arena *arena, Location loc, Expr_Node *value, struct Expr_Match_Arm *arms, size_t len);

MAC_HOT Expr_Node *expr_let_new(Arena *arena, Location loc, Sized_Str name, Type hint, Expr_Node *value, Expr_Node *expr);

MAC_HOT Expr_Node *expr_fun_new(Arena *arena, Location loc, Sized_Str name, Type hint, Sized_Str *pars,
	 						size_t len, Expr_Node *body, Expr_Node *expr);

MAC_HOT Expr_Node *expr_app_new(Arena *arena, Location loc, Expr_Node *fun, Expr_Node **args, size_t len);

MAC_HOT Expr_Node *expr_ident_new(Arena *arena, Location loc, Sized_Str name);

MAC_HOT Expr_Node *expr_unary_new(Arena *arena, Location loc, Token_Kind op, Expr_Node *lhs);

MAC_HOT Expr_Node *expr_binary_new(Arena *arena, Location loc, Token_Kind op, Expr_Node *rhs, Expr_Node *lhs);

MAC_HOT Expr_Node *expr_tuple_new(Arena *arena, Location loc, Expr_Node **items, size_t len);

MAC_HOT Expr_Node *expr_const_new(Arena *arena, Location loc, Const_Value value);

#endif
