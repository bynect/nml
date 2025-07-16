#ifndef NML_DBG_H
#define NML_DBG_H

#include "nml_ast.h"
#include "nml_parse.h"
#include "nml_err.h"
#include "nml_patn.h"
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

void dbg_dump_sized_str(const Sized_Str str);

void dbg_dump_hash_64(const Hash_64 hsh);

void dbg_dump_hash_32(const Hash_32 hsh);

void dbg_dump_hash_default(const Hash_Default hsh);

void dbg_dump_value(const Value val);

void dbg_dump_box(const Box *box);

void dbg_dump_const(const Const_Value value);

void dbg_dump_ast(const Ast_Top *ast);

void dbg_dump_ast_node(const Ast_Node *node, uint32_t depth, bool last);

void dbg_dump_ast_if(const Ast_If *node, uint32_t depth, bool last);

void dbg_dump_ast_match(const Ast_Match *node, uint32_t depth, bool last);

void dbg_dump_ast_let(const Ast_Let *node, uint32_t depth, bool last);

void dbg_dump_ast_fun(const Ast_Fun *node, uint32_t depth, bool last);

void dbg_dump_ast_app(const Ast_App *node, uint32_t depth, bool last);

void dbg_dump_ast_ident(const Ast_Ident *node, uint32_t depth, bool last);

void dbg_dump_ast_unary(const Ast_Unary *node, uint32_t depth, bool last);

void dbg_dump_ast_binary(const Ast_Binary *node, uint32_t depth, bool last);

void dbg_dump_ast_tuple(const Ast_Tuple *node, uint32_t depth, bool last);

void dbg_dump_ast_const(const Ast_Const *node, uint32_t depth, bool last);

void dbg_dump_patn_node(const Patn_Node *node, uint32_t depth, bool last);

void dbg_dump_patn_ident(const Patn_Ident *node, uint32_t depth, bool last);

void dbg_dump_patn_tuple(const Patn_Tuple *node, uint32_t depth, bool last);

void dbg_dump_patn_const(const Patn_Const *node, uint32_t depth, bool last);

#endif
