#ifndef NML_DUMP_H
#define NML_DUMP_H

#include "nml_ast.h"
#include "nml_expr.h"
#include "nml_dbg.h"
#include "nml_ir.h"
#include "nml_parse.h"
#include "nml_err.h"
#include "nml_patn.h"
#include "nml_sig.h"
#include "nml_str.h"
#include "nml_lex.h"
#include "nml_mem.h"
#include "nml_type.h"
#include "nml_hash.h"
#include "nml_mod.h"
#include "nml_block.h"

void dump_source(const Source *src);

void dump_location(Location loc);

void dump_token(const Token *tok);

void dump_lexer(Lexer *lex);

void dump_type_tag(const Type_Tag tag);

void dump_type(const Type type);

void dump_type(const Type type);

void dump_error(Error_List *err);

void dump_arena(const Arena *arena);

void dump_sized_str(const Sized_Str str);

void dump_hash_64(const Hash_64 hsh);

void dump_hash_32(const Hash_32 hsh);

void dump_hash_default(const Hash_Default hsh);

void dump_const_plain(const Const_Value value);

void dump_const(const Const_Value value);

void dump_ast(const Ast_Top *ast);

void dump_ast_node(const Ast_Node node, uint32_t depth, bool last);

void dump_expr_node(const Expr_Node *node, uint32_t depth, bool last);

void dump_expr_if(const Expr_If *node, uint32_t depth, bool last);

void dump_expr_match(const Expr_Match *node, uint32_t depth, bool last);

void dump_expr_let(const Expr_Let *node, uint32_t depth, bool last);

void dump_expr_fun(const Expr_Fun *node, uint32_t depth, bool last);

void dump_expr_app(const Expr_App *node, uint32_t depth, bool last);

void dump_expr_ident(const Expr_Ident *node, uint32_t depth, bool last);

void dump_expr_binary(const Expr_Binary *node, uint32_t depth, bool last);

void dump_expr_tuple(const Expr_Tuple *node, uint32_t depth, bool last);

void dump_expr_const(const Expr_Const *node, uint32_t depth, bool last);

void dump_sig_node(const Sig_Node *node, uint32_t depth, bool last);

void dump_sig_type(const Sig_Type *node, uint32_t depth, bool last);

void dump_patn_node(const Patn_Node *node, uint32_t depth, bool last);

void dump_patn_ident(const Patn_Ident *node, uint32_t depth, bool last);

void dump_patn_tuple(const Patn_Tuple *node, uint32_t depth, bool last);

void dump_patn_const(const Patn_Const *node, uint32_t depth, bool last);

void dump_ir_type(const Ir_Type *type);

void dump_ir_value(const Ir_Value *value);

void dump_instr(const Instr *instr);

void dump_instr_unary(Instr_Kind kind, Instr_Unary instr);

void dump_instr_binary(Instr_Kind kind, Instr_Binary instr);

void dump_instr_load(Instr_Load instr);

void dump_instr_store(Instr_Store instr);

void dump_instr_alloca(Instr_Alloca instr);

void dump_instr_gep(Instr_Gep instr);

void dump_instr_br(Instr_Br instr);

void dump_instr_condbr(Instr_Condbr instr);

void dump_instr_ret(Instr_Ret instr);

void dump_instr_call(Instr_Call instr);

void dump_block(const Block *block);

void dump_function(const Function *fun);

void dump_linkage(const Linkage link);

void dump_definition(const Definition *def);

void dump_module(const Module *mod);

#endif
