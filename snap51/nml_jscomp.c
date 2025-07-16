#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#include "nml_expr.h"
#include "nml_jscomp.h"
#include "nml_dbg.h"
#include "nml_decl.h"
#include "nml_err.h"
#include "nml_fmt.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"

static MAC_INLINE inline uint32_t jscompiler_expr(Js_Compiler *comp, Id_Tab *scope, Expr_Node *node);

static MAC_INLINE inline void
jscompiler_error(Js_Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
}

static MAC_INLINE inline uint32_t
jscompiler_shadow(Id_Tab *scope, Sized_Str name)
{
	uint32_t id = 0;
	id_tab_get(scope, name, &id);
	id_tab_insert(scope, name, ++id);
	return id;
}

static MAC_INLINE inline uint32_t
jscompiler_temporary(Js_Compiler *comp)
{
	return comp->tmp++;
}

static MAC_HOT uint32_t
jscompiler_expr_if(Js_Compiler *comp, Id_Tab *scope, Expr_If *node)
{
	uint32_t tmp = jscompiler_temporary(comp);

	uint32_t cond = jscompiler_expr(comp, scope, node->cond);
	uint32_t b_then = jscompiler_expr(comp, scope, node->b_then);
	uint32_t b_else = jscompiler_expr(comp, scope, node->b_else);

	fprintf(stdout, "var $tmp$%u;\n"
					"if ($tmp$%u)\n"
					"{\n"
					"$tmp$%u = $tmp$%u;\n"
					"} else {\n"
					"$tmp$%u = $tmp$%u;\n"
					"}\n",
					tmp, cond,
					tmp, b_then,
					tmp, b_else);

	return tmp;
}

static MAC_HOT uint32_t
jscompiler_expr_match(Js_Compiler *comp, Id_Tab *scope, Expr_Match *node)
{
	jscompiler_error(comp, node->node.loc, STR_STATIC("Not implemented"));
	return 0;
}

static MAC_HOT uint32_t
jscompiler_expr_let(Js_Compiler *comp, Id_Tab *scope, Expr_Let *node)
{
	jscompiler_error(comp, node->node.loc, STR_STATIC("Not implemented"));
	return 0;
}

static MAC_HOT uint32_t
jscompiler_expr_fun(Js_Compiler *comp, Id_Tab *scope, Expr_Fun *node)
{
	uint32_t id = 0;
	bool shad = id_tab_get(scope, node->name, &id);
	id_tab_insert(scope, node->name, id + 1);
	fprintf(stdout, "var %.*s$%u = ",
			(uint32_t)node->name.len, node->name.str, id + 1);

	Id_Tab sub;
	id_tab_init(&sub, comp->seed);
	id_tab_union(scope, &sub);

	for (size_t i = 0; i < node->len; ++i)
	{
		fprintf(stdout, "%.*s$%u => ",
			(uint32_t)node->pars[i].len, node->pars[i].str,
			jscompiler_shadow(&sub, node->pars[i]));
	}

	fprintf(stdout, "{\n");
	uint32_t body = jscompiler_expr(comp, &sub, node->body);
	fprintf(stdout, "return $tmp$%u; };\n", body);

	id_tab_free(&sub);

	uint32_t tmp = jscompiler_temporary(comp);
	fprintf(stdout, "var $tmp$%u = $tmp$%u;\n", tmp,
			jscompiler_expr(comp, scope, node->expr));

	if (shad)
	{
		id_tab_insert(scope, node->name, id);
	}
	else
	{
		id_tab_remove(scope, node->name);
	}
	return tmp;
}

static MAC_HOT uint32_t
jscompiler_expr_lambda(Js_Compiler *comp, Id_Tab *scope, Expr_Lambda *node)
{
	Id_Tab sub;
	id_tab_init(&sub, comp->seed);
	id_tab_union(scope, &sub);

	uint32_t tmp = jscompiler_temporary(comp);
	fprintf(stdout, "var $tmp$%u = (", tmp);

	for (size_t i = 0; i < node->len; ++i)
	{
		fprintf(stdout, "%.*s$%u => ",
			(uint32_t)node->pars[i].len, node->pars[i].str,
			jscompiler_shadow(&sub, node->pars[i]));
	}

	fprintf(stdout, "{\n");
	uint32_t body = jscompiler_expr(comp, &sub, node->body);
	fprintf(stdout, "return $tmp$%u; };\n", body);

	id_tab_free(&sub);
	return tmp;
}

static MAC_HOT uint32_t
jscompiler_expr_app(Js_Compiler *comp, Id_Tab *scope, Expr_App *node)
{
	uint32_t app[node->len + 1];
	app[0] = jscompiler_expr(comp, scope, node->fun);

	for (size_t i = 0; i < node->len; ++i)
	{
		app[i + 1] = jscompiler_expr(comp, scope, node->args[i]);
	}

	uint32_t tmp = jscompiler_temporary(comp);
	fprintf(stdout, "var $tmp$%u = ", tmp);

	for (size_t i = 0; i < node->len + 1; ++i)
	{
		fprintf(stdout, "($tmp$%u)", app[i]);
	}

	fprintf(stdout, ";\n");
	return tmp;
}

static MAC_HOT uint32_t
jscompiler_expr_ident(Js_Compiler *comp, Id_Tab *scope, Expr_Ident *node)
{
	Sized_Str name;
	switch (node->name.str[0])
	{
		case '+':
			name = STR_STATIC("$add");
			break;

		case '-':
			name = STR_STATIC("$sub");
			break;

		case '*':
			name = STR_STATIC("$mul");
			break;

		case '/':
			name = STR_STATIC("$div");
			break;

		case '=':
			name = STR_STATIC("$eq");
			break;

		default:
			name = node->name;
			break;
	}

	uint32_t id = 0;
	id_tab_get(scope, name, &id);

	uint32_t tmp = jscompiler_temporary(comp);
	fprintf(stdout, "var $tmp$%u = %.*s$%u;\n",
			tmp, (uint32_t)name.len, name.str, id);
	return tmp;
}

static MAC_HOT uint32_t
jscompiler_expr_tuple(Js_Compiler *comp, Id_Tab *scope, Expr_Tuple *node)
{
	uint32_t tmp = jscompiler_temporary(comp);
	fprintf(stdout, "var $tmp$%u = [(", tmp);

	jscompiler_expr(comp, scope, node->items[0]);
	fprintf(stdout, "), ");

	for (size_t i = 1; i < node->len; ++i)
	{
		fprintf(stdout, "(");
		jscompiler_expr(comp, scope, node->items[i - 1]);
		fprintf(stdout, "), ");
	}

	fprintf(stdout, "];\n");
	return tmp;
}

static MAC_HOT uint32_t
jscompiler_expr_const(Js_Compiler *comp, Id_Tab *scope, Expr_Const *node)
{
	uint32_t tmp = jscompiler_temporary(comp);
	fprintf(stdout, "var $tmp$%u = ", tmp);

	switch (node->value.kind)
	{
		case CONST_UNIT:
			fprintf(stdout, "null");
			break;

		case CONST_INT:
			fprintf(stdout, "%d", node->value.c_int);
			break;

		case CONST_FLOAT:
			fprintf(stdout, "%lf", node->value.c_float);
			break;

		case CONST_STR:
			fprintf(stdout, "\"%.*s\"",
					(uint32_t)node->value.c_str.len, node->value.c_str.str);
			break;

		case CONST_CHAR:
			fprintf(stdout, "\"%hhx\"", node->value.c_char);
			break;

		case CONST_BOOL:
			fprintf(stdout, "%s", node->value.c_bool ? "true" : "false");
			break;
	}

	fprintf(stdout, ";\n");
	return tmp;
}

static MAC_INLINE inline uint32_t
jscompiler_expr(Js_Compiler *comp, Id_Tab *scope, Expr_Node *node)
{
	switch (node->kind)
	{
		case EXPR_IF:
			return jscompiler_expr_if(comp, scope, (Expr_If *)node);

		case EXPR_MATCH:
			return jscompiler_expr_match(comp, scope, (Expr_Match *)node);

		case EXPR_LET:
			return jscompiler_expr_let(comp, scope, (Expr_Let *)node);

		case EXPR_FUN:
			return jscompiler_expr_fun(comp, scope, (Expr_Fun *)node);

		case EXPR_LAMBDA:
			return jscompiler_expr_lambda(comp, scope, (Expr_Lambda *)node);

		case EXPR_APP:
			return jscompiler_expr_app(comp, scope, (Expr_App *)node);

		case EXPR_IDENT:
			return jscompiler_expr_ident(comp, scope, (Expr_Ident *)node);

		case EXPR_TUPLE:
			return jscompiler_expr_tuple(comp, scope, (Expr_Tuple *)node);

		case EXPR_CONST:
			return jscompiler_expr_const(comp, scope, (Expr_Const *)node);
	}
}

static MAC_HOT void
jscompiler_decl_let(Js_Compiler *comp, Decl_Let *node)
{
	uint32_t id = jscompiler_shadow(&comp->glob, node->name);
	fprintf(stdout, "var %.*s$%u;\n",
			(uint32_t)node->name.len, node->name.str, id);

	fprintf(stdout, "%.*s$%u = $tmp$%u;\n",
			(uint32_t)node->name.len, node->name.str, id,
			jscompiler_expr(comp, &comp->glob, node->value));
}

static MAC_HOT void
jscompiler_decl_fun(Js_Compiler *comp, Decl_Fun *node)
{
	fprintf(stdout, "var %.*s$%u = ",
			(uint32_t)node->name.len, node->name.str,
			jscompiler_shadow(&comp->glob, node->name));

	Id_Tab scope;
	id_tab_init(&scope, comp->seed);
	id_tab_union(&comp->glob, &scope);

	for (size_t i = 0; i < node->len; ++i)
	{
		fprintf(stdout, "%.*s$%u => ",
			(uint32_t)node->pars[i].len, node->pars[i].str,
			jscompiler_shadow(&scope, node->pars[i]));
	}

	fprintf(stdout, "{\n");
	uint32_t body = jscompiler_expr(comp, &scope, node->body);
	fprintf(stdout, "return $tmp$%u; };\n", body);

	id_tab_free(&scope);
}

static MAC_INLINE inline void
jscompiler_decl(Js_Compiler *comp, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			jscompiler_decl_let(comp, (Decl_Let *)node);
			break;

		case DECL_FUN:
			jscompiler_decl_fun(comp, (Decl_Fun *)node);
			break;

		case DECL_DATA:
			break;

		case DECL_TYPE:
			break;
	}
}

void
jscompiler_init(Js_Compiler *comp, Arena *arena, Error_List *err, Hash_Default seed)
{
	comp->arena = arena;
	comp->err = err;
	comp->succ = true;

	comp->seed = seed;
	id_tab_init(&comp->glob, seed);
	comp->tmp = 0;
}

void
jscompiler_free(Js_Compiler *comp)
{
	id_tab_free(&comp->glob);
}

MAC_HOT bool
jscompiler_pass(Js_Compiler *comp, Ast_Top *ast)
{
	fprintf(stdout, "'use strict';\n");

	struct { char op; char *name; } ops[] = {
		{ '+', "$add" },
		{ '-', "$sub" },
		{ '*', "$mul" },
		{ '/', "$div" },
	};

	for (size_t i = 0; i < sizeof(ops) / sizeof(ops[0]); ++i)
	{
		fprintf(stdout, "function %s$0($1)\n"
						"{\n"
						"	return $2 => $1 %c $2;\n"
						"}\n\n",
						ops[i].name, ops[i].op);
		id_tab_insert(&comp->glob, STR_WLEN(&ops[i].op, 1), 0);
	}

	fprintf(stdout, "function $eq$0($1)\n"
					"{\n"
					"	return $2 => $1 === $2;\n"
					"}\n\n");
	id_tab_insert(&comp->glob, STR_STATIC("="), 0);

	bool succ = true;
	for (size_t i = 0; i < ast->len; ++i)
	{
		jscompiler_decl(comp, ast->decls[i]);

		if (!comp->succ)
		{
			succ = false;
			comp->succ = true;
		}
	}

	for (size_t i = 0; i < comp->glob.size; ++i)
	{
		Id_Tab_Buck *buck = comp->glob.bucks + i;
		if (buck->set)
		{
			Sized_Str name = buck->hk.k;
			if (!isalpha(name.str[0]) && name.str[0] != '_')
			{
				continue;
			}

			fprintf(stdout, "exports.%.*s = %.*s$%u;\n",
							(uint32_t)name.len, name.str,
							(uint32_t)name.len, name.str,
							buck->v);
		}
	}

	return succ;
}
