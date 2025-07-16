#ifndef NML_DUMP_H
#define NML_DUMP_H

#include "nml_ast.h"
#include "nml_expr.h"
#include "nml_dbg.h"
#include "nml_parse.h"
#include "nml_err.h"
#include "nml_patn.h"
#include "nml_decl.h"
#include "nml_str.h"
#include "nml_lex.h"
#include "nml_mem.h"
#include "nml_type.h"
#include "nml_hash.h"

void dump_source(const Source *src);

void dump_location(Location loc);

void dump_token(const Token *tok);

void dump_lexer(Lexer *lex);

void dump_type_tag(const Type_Tag tag);

void dump_type(const Type type);

void dump_type(const Type type);

void dump_type_scheme(const Type_Scheme scheme);

void dump_error(Error_List *err);

void dump_arena(const Arena *arena);

void dump_sized_str(const Sized_Str str);

void dump_hash_64(const Hash_64 hsh);

void dump_hash_32(const Hash_32 hsh);

void dump_hash_default(const Hash_Default hsh);

void dump_const_plain(const Const_Value value);

void dump_const(const Const_Value value);

void dump_ast(const Ast_Top *ast);

void dump_expr_node(const Expr_Node *node, uint32_t depth, bool last);

void dump_expr_if(const Expr_If *node, uint32_t depth, bool last);

void dump_expr_match(const Expr_Match *node, uint32_t depth, bool last);

void dump_expr_let(const Expr_Let *node, uint32_t depth, bool last);

void dump_expr_fun(const Expr_Fun *node, uint32_t depth, bool last);

void dump_expr_lambda(const Expr_Lambda *node, uint32_t depth, bool last);

void dump_expr_app(const Expr_App *node, uint32_t depth, bool last);

void dump_expr_ident(const Expr_Ident *node, uint32_t depth, bool last);

void dump_expr_tuple(const Expr_Tuple *node, uint32_t depth, bool last);

void dump_expr_const(const Expr_Const *node, uint32_t depth, bool last);

void dump_decl_node(const Decl_Node *node, uint32_t depth, bool last);

void dump_decl_let(const Decl_Let *node, uint32_t depth, bool last);

void dump_decl_fun(const Decl_Fun *node, uint32_t depth, bool last);

void dump_decl_data(const Decl_Data *node, uint32_t depth, bool last);

void dump_decl_type(const Decl_Type *node, uint32_t depth, bool last);

void dump_patn_node(const Patn_Node *node, uint32_t depth, bool last);

void dump_patn_ident(const Patn_Ident *node, uint32_t depth, bool last);

void dump_patn_constr(const Patn_Constr *node, uint32_t depth, bool last);

void dump_patn_as(const Patn_As *node, uint32_t depth, bool last);

void dump_patn_tuple(const Patn_Tuple *node, uint32_t depth, bool last);

void dump_patn_const(const Patn_Const *node, uint32_t depth, bool last);

#endif
