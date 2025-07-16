#include <bits/stdint-uintn.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "nml_ast.h"
#include "nml_const.h"
#include "nml_dump.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_mem.h"
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

void
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
	Region *ptr = arena->start;
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
dump_symbol(const Symbol *sym)
{
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

void
dump_value(const Value val)
{
	if (VAL_ISBOX(val))
	{
		dump_box(VAL_UNBOX(val));
	}
	else if (VAL_ISUNIT(val))
	{
		fprintf(stdout, "()");
	}
	else if (VAL_ISINT(val))
	{
		fprintf(stdout, "%d", VAL_UNINT(val));
	}
	else if (VAL_ISFLOAT(val))
	{
		fprintf(stdout, "%g", VAL_UNFLOAT(val));
	}
	else if (VAL_ISCHAR(val))
	{
		fprintf(stdout, "%c", VAL_UNCHAR(val));
	}
	else if (VAL_ISTRUE(val))
	{
		fprintf(stdout, "%s", "true");
	}
	else if (VAL_ISFALSE(val))
	{
		fprintf(stdout, "%s", "false");
	}
	fputc('\n', stdout);
}

void
dump_box(const Box *box)
{
	switch (box->kind)
	{
		case BOX_FUN:
			fprintf(stdout, "Box_Fun");
			break;

		case BOX_TUPLE:
			fprintf(stdout, "Box_Tuple `%zu items`", ((Box_Tuple *)box)->len);
			for (size_t i = 0; i < ((Box_Tuple *)box)->len; ++i)
			{
				dump_value(((Box_Tuple *)box)->items[i]);
			}
			break;

		case BOX_STR:
			fprintf(stdout, "Box_Str `%.*s`",
					(uint32_t)((Box_Str *)box)->str.len,
					((Box_Str *)box)->str.str);
			break;
	}
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
