#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "nml_ast.h"
#include "nml_build.h"
#include "nml_comp.h"
#include "nml_const.h"
#include "nml_dbg.h"
#include "nml_err.h"
#include "nml_fun.h"
#include "nml_hash.h"
#include "nml_ir.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_mod.h"
#include "nml_str.h"
#include "nml_type.h"

static MAC_HOT Variable compiler_node(Compiler *comp, Ast_Node *node,
									Var_Tab *ctx, Function *fun, Block **block);

static MAC_INLINE inline void
compiler_error(Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
}

static MAC_HOT inline Variable
compiler_if(Compiler *comp, Ast_If *node, Var_Tab *ctx, Function *fun, Block **block)
{
	Variable cond = compiler_node(comp, node->cond, ctx, fun, block);
	Variable addr = builder_alloc(&comp->build, *block, node->node.type);

	Block *b_then = builder_block(&comp->build, fun, NULL);
	Variable then = compiler_node(comp, node->then, ctx, fun, &b_then);

	Block *b_else = builder_block(&comp->build, fun, NULL);
	Variable m_else = node->m_else == NULL ? comp->unit :
					compiler_node(comp, node->m_else, ctx, fun, &b_else);

	builder_cond_br(&comp->build, *block, cond, b_then->id, b_else->id);

	Variable yield = builder_local(&comp->build);
	Instr *entry = instr_load_new(comp->arena, node->node.type, addr, yield);
	Block *next = builder_block(&comp->build, fun, entry);

	builder_store(&comp->build, b_then, node->node.type, addr, then);
	builder_br(&comp->build, b_then, next->id);

	builder_store(&comp->build, b_else, node->node.type, addr, m_else);
	builder_br(&comp->build, b_else, next->id);

	*block = next;
	return yield;
}

static MAC_HOT inline Variable
compiler_match(Compiler *comp, Ast_Match *node, Var_Tab *ctx, Function *fun, Block **block)
{
	compiler_error(comp, node->node.loc, STR_STATIC("Pattern matching not implemented"));
	return VAR_ERR;
}

static MAC_HOT inline Variable
compiler_let(Compiler *comp, Ast_Let *node, Var_Tab *ctx, Function *fun, Block **block)
{
	return VAR_ERR;
}

static MAC_HOT inline Variable
compiler_fun(Compiler *comp, Ast_Fun *node, Var_Tab *ctx, Function *fun, Block **block)
{
//	Function fun;
//	function_init(&fun, node->len);
//	builder_init(&comp->build, comp->arena, &fun, comp->mod);
//
//	compiler_node(comp, node->body, ctx, fun, block);
//	module_fun(comp->mod, comp->arena, node->name, LINK_GLOB, node->node.type, fun);
//
//	if (node->expr != NULL)
//	{
//		compiler_node(comp, node->expr, ctx, fun, block);
//	}
	return VAR_ERR;
}

static MAC_HOT inline Variable
compiler_app(Compiler *comp, Ast_App *node, Var_Tab *ctx, Function *fun, Block **block)
{
	Variable callee = compiler_node(comp, node->fun, ctx, fun, block);
	Variable *args = arena_alloc(comp->arena, node->len * sizeof(Variable));
	for (size_t i = 0; i < node->len; ++i)
	{
		args[i] = compiler_node(comp, node->args[i], ctx, fun, block);
	}

	return builder_call(&comp->build, *block, node->node.type,
						callee, args, node->len);
}

static MAC_HOT inline Variable
compiler_ident(Compiler *comp, Ast_Ident *node, Var_Tab *ctx, Function *fun, Block **block)
{
	Variable var;
	if (!var_tab_get(ctx, node->name, &var))
	{
		const Type_Tag tag = TYPE_UNTAG(node->node.type);

		if (tag == TAG_FUN)
		{
			module_fun(comp->mod, comp->arena, node->name, LINK_EXT,
						node->node.type, (Function) {0});
		}
		else if (tag == TAG_TUPLE)
		{
			compiler_error(comp, node->node.loc, STR_STATIC("Tuples not implemented"));
			return VAR_ERR;
		}
		else
		{
			module_var(comp->mod, comp->arena, node->name, LINK_EXT, node->node.type);
		}

		var = VAR_STRGLOB(node->name);
	}
	return var;
}

static MAC_HOT inline Variable
compiler_unary(Compiler *comp, Ast_Unary *node, Var_Tab *ctx, Function *fun, Block **block)
{
	Variable lhs = compiler_node(comp, node->lhs, ctx, fun, block);

	if (node->op == TOK_MINUS)
	{
		if (node->node.type == TYPE_INT)
		{
			return builder_unary(&comp->build, *block, INSTR_INEG, TYPE_INT, lhs);
		}
		else if (node->node.type == TYPE_FLOAT)
		{
			return builder_unary(&comp->build, *block, INSTR_FNEG, TYPE_FLOAT, lhs);
		}
	}
	else if (node->op == TOK_BANG)
	{
		return builder_unary(&comp->build, *block, INSTR_NOT, TYPE_BOOL, lhs);
	}

	compiler_error(comp, node->node.loc, STR_STATIC("Invalid unary operation"));
	return VAR_ERR;
}

static MAC_HOT inline Variable
compiler_binary(Compiler *comp, Ast_Binary *node, Var_Tab *ctx, Function *fun, Block **block)
{
	if (MAC_UNLIKELY(node->op == TOK_AMPAMP || node->op == TOK_BARBAR))
	{
		compiler_error(comp, node->node.loc,
						STR_STATIC("Short circuiting operators not implemented"));
		return VAR_ERR;
	}

	Variable rhs = compiler_node(comp, node->rhs, ctx, fun, block);
	Variable lhs = compiler_node(comp, node->lhs, ctx, fun, block);

	switch (node->op)
	{
		case TOK_PLUS:
			return builder_binary(&comp->build, *block, INSTR_IADD, TYPE_INT, rhs, lhs);

		case TOK_MINUS:
			return builder_binary(&comp->build, *block, INSTR_ISUB, TYPE_INT, rhs, lhs);

		case TOK_STAR:
			return builder_binary(&comp->build, *block, INSTR_IMUL, TYPE_INT, rhs, lhs);

		case TOK_SLASH:
			return builder_binary(&comp->build, *block, INSTR_IDIV, TYPE_INT, rhs, lhs);

		case TOK_PERC:
			return builder_binary(&comp->build, *block, INSTR_IREM, TYPE_INT, rhs, lhs);

		case TOK_PLUSF:
			return builder_binary(&comp->build, *block, INSTR_FADD, TYPE_FLOAT, rhs, lhs);

		case TOK_MINUSF:
			return builder_binary(&comp->build, *block, INSTR_FSUB, TYPE_FLOAT, rhs, lhs);

		case TOK_STARF:
			return builder_binary(&comp->build, *block, INSTR_FMUL, TYPE_FLOAT, rhs, lhs);

		case TOK_SLASHF:
			return builder_binary(&comp->build, *block, INSTR_FDIV, TYPE_FLOAT, rhs, lhs);

		case TOK_PERCF:
			return builder_binary(&comp->build, *block, INSTR_FREM, TYPE_FLOAT, rhs, lhs);

		case TOK_GT:
			if (node->node.type == TYPE_INT)
			{
				return builder_binary(&comp->build, *block, INSTR_IGT, TYPE_INT, rhs, lhs);
			}
			else
			{
				return builder_binary(&comp->build, *block, INSTR_FGT, TYPE_FLOAT, rhs, lhs);
			}
			break;

		case TOK_LT:
			if (node->node.type == TYPE_INT)
			{
				return builder_binary(&comp->build, *block, INSTR_ILT, TYPE_INT, rhs, lhs);
			}
			else
			{
				return builder_binary(&comp->build, *block, INSTR_FLT, TYPE_FLOAT, rhs, lhs);
			}
			break;

		case TOK_GTE:
			if (node->node.type == TYPE_INT)
			{
				return builder_binary(&comp->build, *block, INSTR_IGE, TYPE_INT, rhs, lhs);
			}
			else
			{
				return builder_binary(&comp->build, *block, INSTR_FGE, TYPE_FLOAT, rhs, lhs);
			}
			break;

		case TOK_LTE:
			if (node->node.type == TYPE_INT)
			{
				return builder_binary(&comp->build, *block, INSTR_ILE, TYPE_INT, rhs, lhs);
			}
			else
			{
				return builder_binary(&comp->build, *block, INSTR_FLE, TYPE_FLOAT, rhs, lhs);
			}
			break;

		case TOK_EQ:
			if (node->node.type == TYPE_INT)
			{
				return builder_binary(&comp->build, *block, INSTR_IEQ, TYPE_INT, rhs, lhs);
			}
			else
			{
				return builder_binary(&comp->build, *block, INSTR_FEQ, TYPE_FLOAT, rhs, lhs);
			}
			break;

		case TOK_NE:
			if (node->node.type == TYPE_INT)
			{
				return builder_binary(&comp->build, *block, INSTR_INE, TYPE_INT, rhs, lhs);
			}
			else
			{
				return builder_binary(&comp->build, *block, INSTR_FNE, TYPE_FLOAT, rhs, lhs);
			}
			break;

		default:
			break;
	}

	compiler_error(comp, node->node.loc, STR_STATIC("Invalid binary operation"));
	return VAR_ERR;
}

static MAC_HOT inline Variable
compiler_tuple(Compiler *comp, Ast_Tuple *node, Var_Tab *ctx, Function *fun, Block **block)
{
	compiler_error(comp, node->node.loc, STR_STATIC("Tuples not implemented"));
	return VAR_ERR;
}

static MAC_HOT inline Variable
compiler_const(Compiler *comp, Ast_Const *node)
{
	return builder_const(&comp->build, node->node.type, node->value);
}

static MAC_HOT Variable
compiler_node(Compiler *comp, Ast_Node *node, Var_Tab *ctx, Function *fun, Block **block)
{
	switch (node->kind)
	{
		case AST_IF:
			return compiler_if(comp, (Ast_If *)node, ctx, fun, block);

		case AST_MATCH:
			return compiler_match(comp, (Ast_Match *)node, ctx, fun, block);

		case AST_LET:
			return compiler_let(comp, (Ast_Let *)node, ctx, fun, block);

		case AST_FUN:
			return compiler_fun(comp, (Ast_Fun *)node, ctx, fun, block);

		case AST_APP:
			return compiler_app(comp, (Ast_App *)node, ctx, fun, block);

		case AST_IDENT:
			return compiler_ident(comp, (Ast_Ident *)node, ctx, fun, block);

		case AST_UNARY:
			return compiler_unary(comp, (Ast_Unary *)node, ctx, fun, block);

		case AST_BINARY:
			return compiler_binary(comp, (Ast_Binary *)node, ctx, fun, block);

		case AST_TUPLE:
			return compiler_tuple(comp, (Ast_Tuple *)node, ctx, fun, block);

		case AST_CONST:
			return compiler_const(comp, (Ast_Const *)node);
	}
	return VAR_ERR;
}

void
compiler_init(Compiler *comp, Arena *arena, Module *mod, Hash_Default seed)
{
	comp->arena = arena;
	comp->mod = mod;
	comp->err = NULL;
	comp->succ = true;
	comp->seed = seed;

	builder_init(&comp->build, comp->arena, comp->mod);
	comp->unit = builder_const(&comp->build, TYPE_UNIT, const_new(CONST_UNIT, NULL));
}

MAC_HOT bool
compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err)
{
	comp->err = err;
	bool succ = true;

	Var_Tab ctx;
	var_tab_init(&ctx, comp->seed);

	Function fun;
	function_init(&fun, 0);
	Block *entry = builder_block(&comp->build, &fun, NULL);

	for (size_t i = 0; i < ast->len; ++i)
	{
		compiler_node(comp, ast->items[i], &ctx, &fun, &entry);
		if (MAC_UNLIKELY(!comp->succ))
		{
			comp->succ = true;
			succ = false;
		}
	}
	builder_ret(&comp->build, entry, comp->unit);

	if (succ)
	{
		module_fun(comp->mod, comp->arena, STR_STATIC("module.main"),
					LINK_GLOB, TYPE_NONE, fun);
	}

	return succ;
}
