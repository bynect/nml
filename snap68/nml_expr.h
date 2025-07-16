#ifndef NML_EXPR_H
#define NML_EXPR_H

#include <stdint.h>
#include <stddef.h>

#include "nml_mac.h"
#include "nml_str.h"
#include "nml_lex.h"
#include "nml_type.h"
#include "nml_const.h"
#include "nml_dbg.h"
#include "nml_tab.h"

typedef struct Patn_Node Patn_Node;

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

typedef struct Expr_Match {
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
	Type hint;
	Expr_Node *value;
	Expr_Node *expr;
} Expr_Let;

typedef struct {
	Expr_Node node;
	Type fun;
	Type hint;
	Sized_Str name;
	Sized_Str *pars;
	size_t len;
	Expr_Node *body;
	Expr_Node *expr;
	uint32_t label;
} Expr_Fun;

typedef struct {
	Expr_Node node;
	Sized_Str *pars;
	size_t len;
	Expr_Node *body;
	uint32_t label;
} Expr_Lambda;

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
	Expr_Node **items;
	size_t len;
} Expr_Tuple;

typedef struct {
	Expr_Node node;
	Const_Value value;
} Expr_Const;

TAB_DECL(expr_tab, Expr, Sized_Str, Expr_Node *)

MAC_HOT Expr_Node *expr_if_new(Arena *arena, Location loc, Expr_Node *cond,
							Expr_Node *b_then, Expr_Node *b_else);

MAC_HOT Expr_Node *expr_match_new(Arena *arena, Location loc, Expr_Node *value,
									struct Expr_Match_Arm *arms, size_t len);

MAC_HOT Expr_Node *expr_let_new(Arena *arena, Location loc, Type hint, Sized_Str name,
								Expr_Node *value, Expr_Node *expr);

MAC_HOT Expr_Node *expr_fun_new(Arena *arena, Location loc, Type hint, Sized_Str name,
							Sized_Str *pars, size_t len, Expr_Node *body, Expr_Node *expr);

MAC_HOT Expr_Node *expr_lambda_new(Arena *arena, Location loc, Sized_Str *pars, size_t len, Expr_Node *body);

MAC_HOT Expr_Node *expr_app_new(Arena *arena, Location loc, Expr_Node *fun, Expr_Node **args, size_t len);

MAC_HOT Expr_Node *expr_ident_new(Arena *arena, Location loc, Sized_Str name);

MAC_HOT Expr_Node *expr_tuple_new(Arena *arena, Location loc, Expr_Node **items, size_t len);

MAC_HOT Expr_Node *expr_const_new(Arena *arena, Location loc, Const_Value value);

// TODO: Add expr_*_copy functions to deep copy expression nodes
//
//MAC_HOT Expr_Node *expr_node_copy(Arena *arena, Expr_Node *node);
//
//MAC_HOT Expr_If *expr_if_copy(Arena *arena, Expr_If *node);
//
//MAC_HOT Expr_Match *expr_match_copy(Arena *arena, Expr_Match *node);
//
//MAC_HOT Expr_Let *expr_let_copy(Arena *arena, Expr_Let *node);
//
//MAC_HOT Expr_Fun *expr_fun_copy(Arena *arena, Expr_Fun *node);
//
//MAC_HOT Expr_Lambda *expr_lambda_copy(Arena *arena, Expr_Lambda *node);
//
//MAC_HOT Expr_App *expr_app_copy(Arena *arena, Expr_App *node);
//
//MAC_HOT Expr_Ident *expr_ident_copy(Arena *arena, Expr_Ident *node);
//
//MAC_HOT Expr_Tuple *expr_tuple_copy(Arena *arena, Expr_Tuple *node);
//
//MAC_HOT Expr_Const *expr_const_copy(Arena *arena, Expr_Const *node);

#endif
