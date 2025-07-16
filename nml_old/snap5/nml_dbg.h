#ifndef NML_DBG_H
#define NML_DBG_H

#include "nml_ast.h"
#include "nml_parse.h"
#include "nml_err.h"
#include "nml_str.h"
#include "nml_lex.h"
#include "nml_mem.h"
#include "nml_type.h"
#include "nml_hash.h"
#include "nml_val.h"
#include "nml_box.h"

void dbg_dump_token(const Token *tok);

void dbg_dump_type_tag(const Type_Tag tag);

void dbg_dump_type(const Type type);

void dbg_dump_lexer(Lexer *lex);

void dbg_dump_error_iter(Error *err, void *ctx);

void dbg_dump_error(Error_List *err);

void dbg_dump_arena(const Arena *arena);

void dbg_dump_const_node(const Const_Node *node, uint32_t depth, bool last);

void dbg_dump_ast_module(const Ast_Module *ast);

void dbg_dump_ast_node(const Ast_Node *node, uint32_t depth, bool last);

void dbg_dump_expr_node(const Expr_Node *node, uint32_t depth, bool last);

void dbg_dump_expr_if(const Expr_If *node, uint32_t depth, bool last);

void dbg_dump_expr_let(const Expr_Let *node, uint32_t depth, bool last);

void dbg_dump_expr_fun(const Expr_Fun *node, uint32_t depth, bool last);

void dbg_dump_expr_app(const Expr_App *node, uint32_t depth, bool last);

void dbg_dump_expr_ident(const Expr_Ident *node, uint32_t depth, bool last);

void dbg_dump_expr_unary(const Expr_Unary *node, uint32_t depth, bool last);

void dbg_dump_expr_binary(const Expr_Binary *node, uint32_t depth, bool last);

void dbg_dump_expr_tuple(const Expr_Tuple *node, uint32_t depth, bool last);

void dbg_dump_expr_const(const Expr_Const *node, uint32_t depth, bool last);

void dbg_dump_sized_str(const Sized_Str str);

void dbg_dump_hash_64(const Hash_64 hsh);

void dbg_dump_hash_32(const Hash_32 hsh);

void dbg_dump_hash_default(const Hash_Default hsh);

void dbg_dump_value(const Value val);

void dbg_dump_box(const Box *box);

#endif
