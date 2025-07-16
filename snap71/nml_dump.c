#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "nml_ast.h"
#include "nml_loc.h"
#include "nml_expr.h"
#include "nml_lit.h"
#include "nml_dump.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_parse.h"
#include "nml_decl.h"
#include "nml_str.h"
#include "nml_type.h"

static const char *toks[] = {
#define TOK(name, str, id, kind) [TOK_ ## name] = kind(str, id),
#define SPEC(str, _) str
#define EXTRA(str, _) "Extra(`" str "`)"
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
dump_source(const Source *src)
{
	fprintf(stdout, "%.*s", (uint32_t)src->name.len, src->name.str);
}

void
dump_location(Location loc)
{
	dump_source(loc.src);
	if (MAC_LIKELY(loc.line != 0))
	{
		fprintf(stdout, ":%d:%d", loc.line, loc.col);
	}
}

void
dump_token(const Token *tok)
{
	if ((tok->kind >= TOK_IDENT && tok->kind <= TOK_CHAR) ||
		tok->kind == TOK_EOF || tok->kind == TOK_ERR)
	{
		fprintf(stdout, "%s(%.*s)\n", toks[tok->kind],
				(uint32_t)tok->str.len, tok->str.str);
	}
	else
	{
		fprintf(stdout, "%s\n", toks[tok->kind]);
	}
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
		Type_Var *var = TYPE_PTR(type);
		fprintf(stdout, "%.*s", (uint32_t)var->var.len, var->var.str);
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
dump_type_scheme(const Type_Scheme scheme)
{
	if (scheme.len != 0)
	{
		fprintf(stdout, "forall");
		for (size_t i = 0; i < scheme.len; ++i)
		{
			fprintf(stdout, " %.*s",
					(uint32_t)scheme.vars[i].len, scheme.vars[i].str);
		}
		fprintf(stdout, ". ");
	}
	dump_type(scheme.type);
}

static inline void
dump_error_iter(Error *err, void *ctx)
{
	MAC_UNUSED(ctx);
	dump_location(err->loc);
	fprintf(stdout, " %s: %.*s\n",
			err->flags & ERR_WARN ? "Warning" : "Error",
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
	size_t size = 0;
	size_t off = 0;
	uint32_t i = 0;

	Region *ptr = arena->head;
	while (ptr != NULL)
	{
		fprintf(stdout, "Region %d: %zu of %zu (+%zu)",
				i++, ptr->off, ptr->size, sizeof(Region));

		if (ptr->lock)
		{
			fprintf(stdout, " (lock @%p)", ptr->mem);
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

void
dump_lit(const Lit_Value lit)
{
	switch (lit.kind)
	{
		case LIT_UNIT:
			fprintf(stdout, "()");
			break;

		case LIT_INT:
			fprintf(stdout, "%d", lit.v_int);
			break;

		case LIT_FLOAT:
			fprintf(stdout, "%g", lit.v_float);
			break;

		case LIT_STR:
			fprintf(stdout, "\"%.*s'\"", (uint32_t)lit.v_str.len, lit.v_str.str);
			break;

		case LIT_CHAR:
			fprintf(stdout, "0x%hhx", lit.v_char);
			break;

		case LIT_BOOL:
			fprintf(stdout, "%c", lit.v_bool ? '1' : '0');
			break;
	}
}

void
dump_lit_pretty(const Lit_Value lit)
{
	switch (lit.kind)
	{
		case LIT_UNIT:
			fprintf(stdout, "Unit `()`\n");
			break;

		case LIT_INT:
			fprintf(stdout, "Int `%d`\n", lit.v_int);
			break;

		case LIT_FLOAT:
			fprintf(stdout, "Float `%g`\n", lit.v_float);
			break;

		case LIT_STR:
			fprintf(stdout, "Str `%.*s`\n", (uint32_t)lit.v_str.len, lit.v_str.str);
			break;

		case LIT_CHAR:
			fprintf(stdout, "Char `0x%hhx`\n", lit.v_char);
			break;

		case LIT_BOOL:
			fprintf(stdout, "Bool `%s`\n", lit.v_bool ? "true" : "false");
			break;
	}
}

static MAC_INLINE inline void
dump_node_pretty(uint32_t depth, bool last, const char *fmt, ...)
{
	fprintf(stdout, "%*s%s ", depth * 2, "", last ? "└" : "├");

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void
dump_ast(const Ast_Top *ast)
{
	fprintf(stdout, "Ast_Top\n");
	if (ast->len != 0)
	{
		for (size_t i = 0; i < ast->len - 1; ++i)
		{
			dump_decl_node(ast->decls[i], 0, false);
		}
		dump_decl_node(ast->decls[ast->len - 1], 0, true);
	}
	else
	{
		dump_expr_node(NULL, 0, true);
	}
}

void
dump_expr_node(const Expr_Node *node, uint32_t depth, bool last)
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
		case EXPR_IF:
			dump_expr_if((Expr_If *)node, depth, last);
			break;

		case EXPR_MATCH:
			dump_expr_match((Expr_Match *)node, depth, last);
			break;

		case EXPR_LET:
			dump_expr_let((Expr_Let *)node, depth, last);
			break;

		case EXPR_FUN:
			dump_expr_fun((Expr_Fun *)node, depth, last);
			break;

		case EXPR_LAMBDA:
			dump_expr_lambda((Expr_Lambda *)node, depth, last);
			break;

		case EXPR_APP:
			dump_expr_app((Expr_App *)node, depth, last);
			break;

		case EXPR_IDENT:
			dump_expr_ident((Expr_Ident *)node, depth, last);
			break;

		case EXPR_CONS:
			dump_expr_cons((Expr_Cons *)node, depth, last);
			break;

		case EXPR_TUPLE:
			dump_expr_tuple((Expr_Tuple *)node, depth, last);
			break;

		case EXPR_LIT:
			dump_expr_lit((Expr_Lit *)node, depth, last);
			break;
	}
}

void
dump_expr_if(const Expr_If *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_If\n");
	dump_node_pretty(depth + 1, false, "cond\n");
	dump_expr_node(node->cond, depth + 2, true);

	dump_node_pretty(depth + 1, false, "b_then\n");
	dump_expr_node(node->b_then, depth + 2, true);

	dump_node_pretty(depth + 1, true, "b_else\n");
	dump_expr_node(node->b_else, depth + 2, true);
}

void
dump_expr_match(const Expr_Match *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_Match\n");
	dump_node_pretty(depth + 1, false, "value\n");
	dump_expr_node(node->value, depth + 2, true);

	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "arm %zu\n", i);
		dump_node_pretty(depth + 2, false, "pattern\n");
		dump_patn_node(node->arms[i].patn, depth + 3, true);

		dump_node_pretty(depth + 2, true, "expr\n");
		dump_expr_node(node->arms[i].expr, depth + 3, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "arm %zu\n", i);
	dump_node_pretty(depth + 2, false, "pattern\n");
	dump_patn_node(node->arms[i].patn, depth + 3, true);

	dump_node_pretty(depth + 2, true, "expr\n");
	dump_expr_node(node->arms[i].expr, depth + 3, true);
}

void
dump_expr_let(const Expr_Let *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Hint ", depth * 2, "");
	dump_type(node->hint);

	dump_node_pretty(depth, last, "Expr_Let `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);

	dump_node_pretty(depth + 1, false, "value\n");
	dump_expr_node(node->value, depth + 2, true);

	dump_node_pretty(depth + 1, true, "expr\n");
	dump_expr_node(node->expr, depth + 2, true);
}

void
dump_expr_fun(const Expr_Fun *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Hint ", depth * 2, "");
	dump_type(node->hint);

	fprintf(stdout, "%*s├ Fun ", depth * 2, "");
	dump_type(node->fun);

	dump_node_pretty(depth, last, "Expr_Fun `%.*s` [%u]\n",
						(uint32_t)node->name.len, node->name.str, node->label);

	const size_t len = node->len - 1;
	for (size_t i = 0; i < len; ++i)
	{
		dump_node_pretty(depth + 1, false, "par %u `%.*s`\n",
							i, (uint32_t)node->pars[i].len, node->pars[i].str);
	}
	dump_node_pretty(depth + 1, false, "par %u `%.*s`\n",
					len, (uint32_t)node->pars[len].len, node->pars[len].str);

	dump_node_pretty(depth + 1, false, "body\n");
	dump_expr_node(node->body, depth + 2, true);

	dump_node_pretty(depth + 1, true, "expr\n");
	dump_expr_node(node->expr, depth + 2, true);
}

void
dump_expr_lambda(const Expr_Lambda *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_Lambda [%u]\n", node->label);

	const size_t len = node->len - 1;
	for (size_t i = 0; i < len; ++i)
	{
		dump_node_pretty(depth + 1, false, "par %u `%.*s`\n",
							i, (uint32_t)node->pars[i].len, node->pars[i].str);
	}
	dump_node_pretty(depth + 1, false, "par %u `%.*s`\n",
						len, (uint32_t)node->pars[len].len, node->pars[len].str);

	dump_node_pretty(depth + 1, true, "body\n");
	dump_expr_node(node->body, depth + 2, true);
}

void
dump_expr_app(const Expr_App *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_App\n");
	dump_node_pretty(depth + 1, false, "fun\n");
	dump_expr_node(node->fun, depth + 2, true);

	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "arg %zu\n", i);
		dump_expr_node(node->args[i], depth + 2, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "arg %zu\n", i);
	dump_expr_node(node->args[i], depth + 2, true);
}

void
dump_expr_ident(const Expr_Ident *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_Ident `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);
}

void
dump_expr_cons(const Expr_Cons *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_Cons `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);

	dump_node_pretty(depth + 1, true, "value\n");
	dump_expr_node(node->value, depth + 2, true);
}

void
dump_expr_tuple(const Expr_Tuple *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_Tuple `%zu items`\n", node->len);
	size_t i = 0;
	while (i < node->len - 1)
	{
		dump_node_pretty(depth + 1, false, "item %zu\n", i);
		dump_expr_node(node->items[i], depth + 2, true);
		++i;
	}

	dump_node_pretty(depth + 1, true, "item %zu\n", i);
	dump_expr_node(node->items[i], depth + 2, true);
}

void
dump_expr_lit(const Expr_Lit *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Expr_Lit\n");
	dump_node_pretty(depth + 1, true, "");
	dump_lit_pretty(node->value);
}

void
dump_decl_node(const Decl_Node *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Location ", depth * 2, "");
	dump_location(node->loc);
	fputc('\n', stdout);

	switch (node->kind)
	{
		case DECL_LET:
			dump_decl_let((Decl_Let *)node, depth, last);
			break;

		case DECL_FUN:
			dump_decl_fun((Decl_Fun *)node, depth, last);
			break;

		case DECL_DATA:
			dump_decl_data((Decl_Data *)node, depth, last);
			break;

		case DECL_TYPE:
			dump_decl_type((Decl_Type *)node, depth, last);
			break;
	}
}

void
dump_decl_let(const Decl_Let *node, uint32_t depth, bool last)
{

	fprintf(stdout, "%*s├ Hint ", depth * 2, "");
	dump_type(node->hint);

	dump_node_pretty(depth, last, "Decl_Let `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);

	dump_node_pretty(depth + 1, true, "value\n");
	dump_expr_node(node->value, depth + 2, true);
}

void
dump_decl_fun(const Decl_Fun *node, uint32_t depth, bool last)
{
	fprintf(stdout, "%*s├ Hint ", depth * 2, "");
	dump_type(node->hint);

	fprintf(stdout, "%*s├ Fun ", depth * 2, "");
	dump_type(node->fun);

	dump_node_pretty(depth, last, "Decl_Fun `%.*s` [%u]\n",
						(uint32_t)node->name.len, node->name.str, node->label);

	const size_t len = node->len - 1;
	for (size_t i = 0; i < len; ++i)
	{
		dump_node_pretty(depth + 1, false, "par %u `%.*s`\n",
							i, (uint32_t)node->pars[i].len, node->pars[i].str);
	}
	dump_node_pretty(depth + 1, false, "par %u `%.*s`\n",
					len, (uint32_t)node->pars[len].len, node->pars[len].str);

	dump_node_pretty(depth + 1, true, "body\n");
	dump_expr_node(node->body, depth + 2, true);
}

void
dump_decl_data(const Decl_Data *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Decl_Data `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);


	const size_t len = node->len - 1;
	for (size_t i = 0; i < len; ++i)
	{
		dump_node_pretty(depth + 1, false, "constr %u `%.*s`\n",
							i, (uint32_t)node->constrs[i].name.len, node->constrs[i].name.str);

		dump_node_pretty(depth + 1, true, "Type ");
		dump_type(node->constrs[i].type);
	}
	dump_node_pretty(depth + 1, false, "constr %u `%.*s`\n",
					len, (uint32_t)node->constrs[len].name.len, node->constrs[len].name.str);

	dump_node_pretty(depth + 1, true, "Type ");
	dump_type(node->constrs[len].type);
}

void
dump_decl_type(const Decl_Type *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Decl_Type `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);

	dump_node_pretty(depth + 1, true, "Type ");
	dump_type(node->type);
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

		case PATN_CONSTR:
			dump_patn_constr((Patn_Constr *)node, depth, last);
			break;

		case PATN_AS:
			dump_patn_as((Patn_As *)node, depth, last);
			break;

		case PATN_TUPLE:
			dump_patn_tuple((Patn_Tuple *)node, depth, last);
			break;

		case PATN_LIT:
			dump_patn_lit((Patn_Lit *)node, depth, last);
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
dump_patn_constr(const Patn_Constr *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Patn_Constr `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);


	if (node->patn != NULL)
	{
		dump_node_pretty(depth + 1, true, "patn\n");
		dump_patn_node(node->patn, depth + 2, true);
	}
	else
	{
		dump_node_pretty(depth, last, "`null`\n");
	}
}

void
dump_patn_as(const Patn_As *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Patn_As `%.*s`\n",
						(uint32_t)node->name.len, node->name.str);


	dump_node_pretty(depth + 1, true, "patn\n");
	dump_patn_node(node->patn, depth + 2, true);
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
dump_patn_lit(const Patn_Lit *node, uint32_t depth, bool last)
{
	dump_node_pretty(depth, last, "Patn_Lit\n");
	dump_node_pretty(depth + 1, true, "");
	dump_lit_pretty(node->value);
}
