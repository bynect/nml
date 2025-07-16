#ifndef NML_DUMP_H
#define NML_DUMP_H

#include "nml_ast.h"
#include "nml_dbg.h"
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

void dump_location(Location loc);

void dump_token(const Token *tok);

void dump_lexer(Lexer *lex);

void dump_type_tag(const Type_Tag tag);

void dump_type(const Type type);

void dump_type(const Type type);

void dump_error_iter(Error *err, void *ctx);

void dump_error(Error_List *err);

void dump_arena(const Arena *arena);

void dump_sized_str(const Sized_Str str);

void dump_hash_64(const Hash_64 hsh);

void dump_hash_32(const Hash_32 hsh);

void dump_hash_default(const Hash_Default hsh);

void dump_value(const Value val);

void dump_box(const Box *box);

void dump_const(const Const_Value value);

void dump_ast(const Ast_Top *ast);

void dump_ast_node(const Ast_Node *node, uint32_t depth, bool last);

void dump_ast_if(const Ast_If *node, uint32_t depth, bool last);

void dump_ast_match(const Ast_Match *node, uint32_t depth, bool last);

void dump_ast_let(const Ast_Let *node, uint32_t depth, bool last);

void dump_ast_fun(const Ast_Fun *node, uint32_t depth, bool last);

void dump_ast_app(const Ast_App *node, uint32_t depth, bool last);

void dump_ast_ident(const Ast_Ident *node, uint32_t depth, bool last);

void dump_ast_unary(const Ast_Unary *node, uint32_t depth, bool last);

void dump_ast_binary(const Ast_Binary *node, uint32_t depth, bool last);

void dump_ast_tuple(const Ast_Tuple *node, uint32_t depth, bool last);

void dump_ast_const(const Ast_Const *node, uint32_t depth, bool last);

void dump_patn_node(const Patn_Node *node, uint32_t depth, bool last);

void dump_patn_ident(const Patn_Ident *node, uint32_t depth, bool last);

void dump_patn_tuple(const Patn_Tuple *node, uint32_t depth, bool last);

void dump_patn_const(const Patn_Const *node, uint32_t depth, bool last);

#endif
