#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "nml_ast.h"
#include "nml_const.h"
#include "nml_dump.h"
#include "nml_err.h"
#include "nml_fun.h"
#include "nml_hash.h"
#include "nml_ir.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_mod.h"
#include "nml_parse.h"
#include "nml_str.h"
#include "nml_type.h"

static const char *toks[] = {
#define TOK(name, str, id, kind) [TOK_ ## name] = kind(str, id),
#define SPEC(str, _) str
#define EXTRA(str, _) str
#define DELIM(str, _) "Delim(`" str "`)"
#define OPER(str, _) "Oper(`" str "`)"
#define LIT(str, _) str
#define KWD(_, id) "Keyword(" #id ")"
#include "nml_tok.h"
#undef SPEC
#undef EXTRA
#undef DELIM
#undef OPER
#undef LIT
#undef KWD
#undef TOK
};

static const char *types[] = {
#define TAG(name, id, _)	[TAG_ ## name] = #id,
#define TYPE
#include "nml_tag.h"
#undef TYPE
#undef TAG
};

static const char *instrs[] = {
#define INSTR(name, id)	[INSTR_ ## name] = #id,
#include "nml_instr.h"
#undef INSTR
};

void
dump_location(Location loc)
{
	fprintf(stdout, "Line %d:%d", loc.line, loc.col);
}

void
dump_token(const Token *tok)
{
	fprintf(stdout, "%s = %.*s\n", toks[tok->kind],
			(uint32_t)tok->str.len, tok->str.str);
}

void
dump_lexer(Lexer *lex)
{
	Token tok;
	while (lexer_next(lex, &tok))
	{
		dump_location(tok.loc);
		fprintf(stdout, ": ");
		dump_token(&tok);
	}
}

static MAC_INLINE inline void
dump_tag_pretty(const Type_Tag tag)
{
	fprintf(stdout, "%s", types[tag]);
}

static MAC_INLINE inline void
dump_type_pretty(const Type type, uint32_t fun_depth, uint32_t tuple_depth)
{
	const Type_Tag tag = TYPE_UNTAG(type);

	if (tag == TAG_FUN)
	{
		const Type_Fun *fun = TYPE_PTR(type);
		if (fun_depth > 0)
		{
			fprintf(stdout, "(");
		}

		dump_type_pretty(fun->par, fun_depth + 1, tuple_depth);
		while (TYPE_UNTAG(fun->ret) == TAG_FUN)
		{
			fun = TYPE_PTR(fun->ret);
			fprintf(stdout, " -> ");
			dump_type_pretty(fun->par, fun_depth + 1, tuple_depth);
		}

		fprintf(stdout, " -> ");
		dump_type_pretty(fun->ret, fun_depth + 1, tuple_depth);
		if (fun_depth > 0)
		{
			fprintf(stdout, ")");
		}
	}
	else if (tag == TAG_TUPLE)
	{
		Type_Tuple *tuple = TYPE_PTR(type);
		if (tuple_depth > 0)
		{
			fprintf(stdout, "(");
		}

		for (size_t i = 0; i < tuple->len - 1; ++i)
		{
			dump_type_pretty(tuple->items[i], fun_depth + 1, tuple_depth + 1);
			fprintf(stdout, " * ");
		}

		dump_type_pretty(tuple->items[tuple->len - 1], fun_depth + 1, tuple_depth + 1);
		if (tuple_depth > 0)
		{
			fprintf(stdout, ")");
		}
	}
	else if (tag == TAG_VAR)
	{
		fprintf(stdout, "V%u", TYPE_TID(type));
	}
	else
	{
		dump_tag_pretty(tag);
	}
}

void
dump_type_tag(const Type_Tag tag)
{
	dump_tag_pretty(tag);
	fprintf(stdout, "\n");
}

void
dump_type(const Type type)
{
	dump_type_pretty(type, 0, 0);
	fprintf(stdout, "\n");
}

static inline void
dump_error_iter(Error *err, void *ctx)
{
	MAC_UNUSED(ctx);
	dump_location(err->loc);
	fprintf(stdout, " %s: %.*s\n",
			err->flags & ERR_WARN ? "Warning" :
			err->flags & ERR_INFO ? "Info" : "Error",
			(uint32_t)err->msg.len, err->msg.str);
}

void
dump_error(Error_List *err)
{
	error_iter(err, dump_error_iter, NULL);
}

void
dump_arena(const Arena *arena)
{
	Region *ptr = arena->head;
	size_t size = 0;
	size_t off = 0;
	uint32_t i = 0;

	while (ptr != NULL)
	{
		fprintf(stdout, "Region %d: %zu of %zu (+%zu)",
				i++, ptr->off, ptr->size, sizeof(Region));

		if (ptr->lock)
		{
			fprintf(stdout, " (lock %p)", ptr->mem);
		}
		fputc('\n', stdout);

		size += ptr->size + sizeof(Region);
		off += ptr->off;
		ptr = ptr->next;
	}

	fprintf(stdout, "Arena use: %zu of %zu\n", off, size);
}

void
dump_sized_str(const Sized_Str str)
{
	fprintf(stdout, "%.*s\n", (uint32_t)str.len, str.str);
}

void
dump_hash_64(const Hash_64 hsh)
{
	fprintf(stdout, "%lu\n", hsh);
}

void
dump_hash_32(const Hash_32 hsh)
{
	fprintf(stdout, "%u\n", hsh);
}

void
dump_hash_default(const Hash_Default hsh)
{
	dump_hash_64((Hash_64)hsh);
}

static inline void
dump_node_pretty(uint32_t depth, bool last, const char *fmt, ...)
{
	fprintf(stdout, "%*s%s ", depth * 2, "", last ? "└" : "├");

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void
dump_const_plain(const Const_Value value)
{
	switch (value.kind)
	{
		case CONST_UNIT:
			fprintf(stdout, "()");
			break;

		case CONST_INT:
			fprintf(stdout, "%d", value.c_int);
			break;

		case CONST_FLOAT:
			fprintf(stdout, "%g", value.c_float);
			break;

		case CONST_STR:
			fprintf(stdout, "\"%.*s'\"", (uint32_t)value.c_str.len, value.c_str.str);
			break;

		case CONST_CHAR:
			fprintf(stdout, "0x%hhx", value.c_char);
			break;

		case CONST_BOOL:
			fprintf(stdout, "%c", value.c_bool ? '1' : '0');
			break;
	}
}

void
dump_const(const Const_Value value)
{
	switch (value.kind)
	{
		case CONST_UNIT:
			fprintf(stdout, "Unit `()`\n");
			break;

		case CONST_INT:
			fprintf(stdout, "Int `%d`\n", value.c_int);
			break;

		case CONST_FLOAT:
			fprintf(stdout, "Float `%g`\n", value.c_float);
			break;

		case CONST_STR:
			fprintf(stdout, "Str `%.*s`\n", (uint32_t)value.c_str.len, value.c_str.str);
			break;

		case CONST_CHAR:
			fprintf(stdout, "Char `0x%hhx`\n", value.c_char);
			break;

		case CONST_BOOL:
			fprintf(stdout, "Bool `%s`\n", value.c_bool ? "true" : "false");
			break;
	}
}

void
dump_ast(const Ast_Top *ast)
{
	fprintf(stdout, "Ast_Top\n");
	if (ast->len != 0)
	{
		for (size_t i = 0; i < ast->len - 1; ++i)
		{
			dump_ast_node(ast->items[i], 0, false);
		}
		dump_ast_node(ast->items[ast->len - 1], 0, true);
	}
	else
	{
		dump_ast_node(NULL, 0, true);
	}
}

void
dump_ast_node(const Ast_Node *node, uint32_t depth, bool last)
{
	if (MAC_UNLIKELY(node == NULL))
	{
		dump_node_pretty(depth, last, "`null`\n");
		return;
	}

	fprintf(stdout, "%*s├ Type ", depth * 2, "");
	dump_type(node->type);

	fprintf(stdout, "%*s├ Location ", depth * 2, "");
	dump_location(node->loc);
	fputc('\n', stdout);

	switch (node->kind)
	{
		case AST_IF:
			dump_ast_if((Ast_If *)node, depth, last);
			break;

		case AST_MATCH:
			dump_ast_match((Ast_Match *)node, depth, last);
			break;

		case AST_LET:
			dump_ast_let((Ast_Let *)node, depth, last);
			break;

		case AST_FUN:
			dump_ast_fun((Ast_Fun *)node, depth, last);
			break;

		case AST_APP:
			dump_ast_app((Ast_App *)node, depth, last);
			break;

		case AST_IDENT:
			dump_ast_ident((Ast_Ident *)node, depth, last);
			break;

		case AST_UNARY:
			dump_ast_unary((Ast_Unary *)node, depth, last);
			break;

		case AST_BINARY:
			dump_ast_binary((Ast_Binary *)node, depth, last);
			break;

		case AST_TUPLE:
			dump_ast_tuple((Ast_Tuple *)node, depth, last);
			break;

		case AST_CONST:
			dump_ast_const((Ast_Const *)node, depth, last);
			break;
	}
}

void
dump_ast_if(const Ast_If *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_If\n");
	dump_node_pretty(depth + 1, false, "cond\n");
	dump_ast_node(node->cond, depth + 2, true);

	dump_node_pretty(depth + 1, false, "then\n");
	dump_ast_node(node->then, depth + 2, true);

	dump_node_pretty(depth + 1, true, "m_else\n");
	dump_ast_node(node->m_else, depth + 2, true);
}

void
dump_ast_match(const Ast_Match *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_Match\n");
	dump_node_pretty(depth + 1, false, "value\n");
	dump_ast_node(node->value, depth + 2, true);

	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "arm %zu\n", i);
		dump_node_pretty(depth + 2, false, "pattern\n");
		dump_patn_node(node->arms[i].patn, depth + 3, true);

		dump_node_pretty(depth + 2, true, "expr\n");
		dump_ast_node(node->arms[i].expr, depth + 3, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "arm %zu\n", i);
	dump_node_pretty(depth + 2, false, "pattern\n");
	dump_patn_node(node->arms[i].patn, depth + 3, true);

	dump_node_pretty(depth + 2, true, "expr\n");
	dump_ast_node(node->arms[i].expr, depth + 3, true);
}

void
dump_ast_let(const Ast_Let *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Hint ", depth * 2, "");
	dump_type(node->hint);

	dump_node_pretty(depth, last, "Ast_Let `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);

	dump_node_pretty(depth + 1, false, "value\n");
	dump_ast_node(node->value, depth + 2, true);

	dump_node_pretty(depth + 1, true, "expr\n");
	dump_ast_node(node->expr, depth + 2, true);
}

void
dump_ast_fun(const Ast_Fun *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Hint ", depth * 2, "");
	dump_type(node->hint);

	dump_node_pretty(depth, last, "Ast_Fun `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);

	if (node->len != 0)
	{
		const size_t len = node->len - 1;
		for (size_t i = 0; i < len; ++i)
		{
			dump_node_pretty(depth + 1, false, "param %u `%.*s`\n",
								i, (uint32_t)node->pars[i].len, node->pars[i].str);
		}
		dump_node_pretty(depth + 1, false, "param %u `%.*s`\n",
							len, (uint32_t)node->pars[len].len, node->pars[len].str);
	}

	dump_node_pretty(depth + 1, false, "body\n");
	dump_ast_node(node->body, depth + 2, true);

	dump_node_pretty(depth + 1, true, "expr\n");
	dump_ast_node(node->expr, depth + 2, true);
}

void
dump_ast_app(const Ast_App *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_App\n");
	dump_node_pretty(depth + 1, false, "fun\n");
	dump_ast_node(node->fun, depth + 2, true);

	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "arg %zu\n", i);
		dump_ast_node(node->args[i], depth + 2, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "arg %zu\n", i);
	dump_ast_node(node->args[i], depth + 2, true);
}

void
dump_ast_ident(const Ast_Ident *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_Ident `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);
}

void
dump_ast_unary(const Ast_Unary *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_Unary %s\n", toks[node->op]);
	dump_node_pretty(depth + 1, false, "lhs\n");
	dump_ast_node(node->lhs, depth + 2, true);
}

void
dump_ast_binary(const Ast_Binary *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_Binary %s\n", toks[node->op]);
	dump_node_pretty(depth + 1, false, "rhs\n");
	dump_ast_node(node->rhs, depth + 2, true);

	dump_node_pretty(depth + 1, true, "lhs\n");
	dump_ast_node(node->lhs, depth + 2, true);
}

void
dump_ast_tuple(const Ast_Tuple *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_Tuple `%zu items`\n", node->len);
	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "item %zu\n", i);
		dump_ast_node(node->items[i], depth + 2, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "item %zu\n", i);
	dump_ast_node(node->items[i], depth + 2, true);
}

void
dump_ast_const(const Ast_Const *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Ast_Const\n");
	dump_node_pretty(depth + 1, true, "");
	dump_const(node->value);
}

void
dump_patn_node(const Patn_Node *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Type ", depth * 2, "");
	dump_type(node->type);

	fprintf(stdout, "%*s├ Location ", depth * 2, "");
	dump_location(node->loc);
	fputc('\n', stdout);

	switch (node->kind)
	{
		case PATN_IDENT:
			dump_patn_ident((Patn_Ident *)node, depth, last);
			break;

		case PATN_TUPLE:
			dump_patn_tuple((Patn_Tuple *)node, depth, last);
			break;

		case PATN_CONST:
			dump_patn_const((Patn_Const *)node, depth, last);
			break;
	}
}

void
dump_patn_ident(const Patn_Ident *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Patn_Ident `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);
}

void
dump_patn_tuple(const Patn_Tuple *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Patn_Tuple `%zu items`\n", node->len);
	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "item %zu\n", i);
		dump_patn_node(node->items[i], depth + 2, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "item %zu\n", i);
	dump_patn_node(node->items[i], depth + 2, true);
}

void
dump_patn_const(const Patn_Const *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Patn_Const\n");
	dump_node_pretty(depth + 1, true, "");
	dump_const(node->value);
}

void
dump_variable(const Variable *var)
{
	switch (var->kind)
	{
		case VAR_LOC:
			fprintf(stdout, "$%d", var->id);
			break;

		case VAR_ADDR:
			fprintf(stdout, "*%d", var->id);
			break;

		case VAR_GLOB:
			fprintf(stdout, "#%.*s", (uint32_t)var->name.len, var->name.str);
			break;
	}
}

void
dump_instr(const Instr *instr)
{
	switch (instr->kind)
	{
		case INSTR_INEG:
		case INSTR_FNEG:
			dump_instr_unary(instr->kind, instr->type, instr->unary);
			break;

		case INSTR_LOAD:
			dump_instr_load(instr->type, instr->load);
			break;

		case INSTR_STORE:
			dump_instr_store(instr->type, instr->store);
			break;

		case INSTR_ALLOC:
			dump_instr_alloc(instr->type, instr->alloc);
			break;

		case INSTR_ELEMOF:
			dump_instr_elemof(instr->type, instr->elemof);
			break;

		case INSTR_ADDROF:
			dump_instr_addrof(instr->type, instr->addrof);
			break;

		case INSTR_BR:
			dump_instr_br(instr->br);
			break;

		case INSTR_RET:
			dump_instr_ret(instr->ret);
			break;

		case INSTR_CALL:
			dump_instr_call(instr->type, instr->call);
			break;

		default:
			dump_instr_binary(instr->kind, instr->type, instr->binary);
			break;
	}
}

void
dump_instr_unary(Instr_Kind kind, Type type, Instr_Unary instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " = %s ", instrs[kind]);
	dump_type_pretty(type, 0, 0);

	fputc(' ', stdout);
	dump_variable(&instr.lhs);
	fputc('\n', stdout);
}

void
dump_instr_binary(Instr_Kind kind, Type type, Instr_Binary instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " = %s ", instrs[kind]);
	dump_type_pretty(type, 0, 0);

	fputc(' ', stdout);
	dump_variable(&instr.rhs);
	fprintf(stdout, ", ");
	dump_variable(&instr.lhs);
	fputc('\n', stdout);
}

void
dump_instr_load(Type type, Instr_Load instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " = load ");
	dump_type_pretty(type, 0, 0);

	fputc(' ', stdout);
	dump_variable(&instr.addr);
	fputc('\n' , stdout);
}

void
dump_instr_store(Type type, Instr_Store instr)
{
	fprintf(stdout, "store ");
	dump_type_pretty(type, 0, 0);
	fputc(' ' , stdout);

	dump_variable(&instr.addr);
	fprintf(stdout, ", ");
	dump_variable(&instr.value);
	fputc('\n' , stdout);
}

void
dump_instr_alloc(Type type, Instr_Alloc instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " =  alloc ");
	dump_type(type);
}

void
dump_instr_elemof(Type type, Instr_Elemof instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " = ");
	dump_type_pretty(type, 0, 0);

	fprintf(stdout, " elemof ");
	dump_variable(&instr.addr);
	fprintf(stdout, ", %d\n", instr.index);
}

void
dump_instr_addrof(Type type, Instr_Addrof instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " = ");
	dump_type_pretty(type, 0, 0);

	fprintf(stdout, " addrof ");
	dump_variable(&instr.value);
	fputc('\n', stdout);
}

void
dump_instr_br(Instr_Br instr)
{
	if (instr.m_cond.id != 0)
	{
		fprintf(stdout, "br ");
		dump_variable(&instr.m_cond);
		fprintf(stdout, ", %%%d, %%%d\n", instr.then, instr.m_else);
	}
	else
	{
		fprintf(stdout, "br %%%d\n", instr.then);
	}
}

void
dump_instr_ret(Instr_Ret instr)
{
	fprintf(stdout, "ret");
	if (instr.value.id != 0)
	{
		fputc(' ', stdout);
		dump_variable(&instr.value);
	}
	fputc('\n', stdout);
}

void
dump_instr_call(Type type, Instr_Call instr)
{
	dump_variable(&instr.yield);
	fprintf(stdout, " = ");
	dump_type_pretty(type, 0, 0);

	fprintf(stdout, " call ");
	dump_variable(&instr.fun);
	fputc('(', stdout);

	dump_variable(&instr.args[0]);
	for (size_t i = 1; i < instr.len; ++i)
	{
		fprintf(stdout, ", ");
		dump_variable(&instr.args[i]);
	}
	fprintf(stdout, ")\n");
}

void
dump_block(const Block *block)
{
	fprintf(stdout, "%%%d:\n", block->id);

	Instr *instr = block->head;
	while (instr != NULL)
	{
		fputc('\t', stdout);
		dump_instr(instr);
		instr = instr->next;
	}
}

void
dump_function(const Function *fun)
{
	fprintf(stdout, "{\n");

	Block *block = fun->head;
	while (block != NULL)
	{
		dump_block(block);
		block = block->next;
	}

	fprintf(stdout, "}\n");
}

void
dump_linkage(const Linkage link)
{
	switch (link)
	{
		case LINK_GLOB:
			fprintf(stdout, "glob");
			break;

		case LINK_STAT:
			fprintf(stdout, "stat");
			break;

		case LINK_EXT:
			fprintf(stdout, "ext");
			break;
	}
}

void
dump_definition(const Definition *def)
{
	if (def->kind == DEF_FUN)
	{
		dump_linkage(def->link);
		fprintf(stdout, " fun #%.*s (", (uint32_t)def->name.len, def->name.str);
		dump_type_pretty(def->type, 0, 0);
		fprintf(stdout, ")\n");

		if (MAC_LIKELY(def->link != LINK_EXT))
		{
			dump_function(&def->fun);
		}
	}
	else
	{
		fprintf(stdout, "#%.*s = ", (uint32_t)def->name.len, def->name.str);
		dump_linkage(def->link);

		if (def->kind == DEF_CONST)
		{
			fprintf(stdout, " const ");
			dump_type_pretty(def->type, 0, 0);
			fputc(' ', stdout);
			dump_const_plain(def->value);
			fputc('\n', stdout);
		}
		else
		{
			fprintf(stdout, " var ");
			dump_type(def->type);
		}
	}
}

static inline void
dump_module_iter(const Definition *def, void *ctx)
{
	MAC_UNUSED(ctx);
	fputc('\n', stdout);
	dump_definition(def);
}

void
dump_module(const Module *mod)
{
	fprintf(stdout, "!module.name = \"%.*s\"\n",
			(uint32_t)mod->name.len, mod->name.str);
	module_iter(mod, dump_module_iter, NULL);
}
