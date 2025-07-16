#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nml_ast.h"
#include "nml_expr.h"
#include "nml_const.h"
#include "nml_mac.h"
#include "nml_err.h"
#include "nml_lex.h"
#include "nml_mem.h"
#include "nml_parse.h"
#include "nml_patn.h"
#include "nml_set.h"
#include "nml_sig.h"
#include "nml_str.h"
#include "nml_type.h"
#include "nml_fmt.h"

typedef void (*const Parser_Fn)(Parser *parse, Expr_Node **node);

#define PFN(name) (Parser_Fn)parser_expr_ ## name

typedef struct {
	const Parser_Fn prefix;
	const Parser_Fn infix;
	const Parser_Prec prec;
	const Parser_Assoc assoc;
} Precedence;

typedef struct {
	const Sized_Str name;
	const Type type;
} Primitive;

static MAC_INLINE inline Type parser_type(Parser *parse);
static MAC_INLINE inline void parser_sig(Parser *parse, Sig_Node **node);
static MAC_INLINE inline void parser_const(Parser *parse, Const_Value *value);
static MAC_INLINE inline void parser_patn(Parser *parse, Patn_Node **node);

static MAC_HOT void parser_expr_group(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_semi(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_uline(Parser *parse, Expr_Node **node);

static MAC_HOT void parser_expr_if(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_match(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_let(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_fun(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_app(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_ident(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_unary(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_binary(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_tuple(Parser *parse, Expr_Node **node);
static MAC_HOT void parser_expr_const(Parser *parse, Expr_Node **node);

static MAC_HOT void parser_expr_prec(Parser *parse, Expr_Node **node, Parser_Prec prec);
static MAC_HOT void parser_expr(Parser *parse, Expr_Node **node);

static const Precedence precs[] = {
	[TOK_EOF]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_ERR]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_BAR]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_AMP]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_COL]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_HASH]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_ULINE]	 = { PFN(uline),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_BTICK]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_LPAR]	 = { PFN(group),	NULL,			PREC_NONE,		ASSOC_NONE	},
	[TOK_RPAR]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_LBRACK] = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_RBRACK] = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_LBRACE] = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_RBRACE] = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_PLUS]	 = { NULL,			PFN(binary),	PREC_TERM,		ASSOC_LEFT 	},
	[TOK_MINUS]	 = { PFN(unary),	PFN(binary),	PREC_TERM,		ASSOC_LEFT 	},
	[TOK_STAR]	 = { NULL,			PFN(binary),	PREC_FACTOR,	ASSOC_LEFT 	},
	[TOK_SLASH]	 = { NULL,			PFN(binary),	PREC_FACTOR,	ASSOC_LEFT 	},
	[TOK_PERC]	 = { NULL,			PFN(binary),	PREC_FACTOR,	ASSOC_LEFT 	},
	[TOK_PLUSF]	 = { NULL,			PFN(binary),	PREC_TERM,		ASSOC_LEFT 	},
	[TOK_MINUSF] = { PFN(unary),	PFN(binary),	PREC_TERM,		ASSOC_LEFT 	},
	[TOK_STARF]	 = { NULL,			PFN(binary),	PREC_FACTOR,	ASSOC_LEFT 	},
	[TOK_SLASHF] = { NULL,			PFN(binary),	PREC_FACTOR,	ASSOC_LEFT 	},
	[TOK_PERCF]	 = { NULL,			PFN(binary),	PREC_FACTOR,	ASSOC_LEFT 	},
	[TOK_CARET]	 = { NULL,			PFN(binary),	PREC_POWER,		ASSOC_RIGHT },
	[TOK_DOT]	 = { NULL,			PFN(binary),	PREC_ACCESS,	ASSOC_NONE 	},
	[TOK_AT]	 = { NULL,			PFN(binary),	PREC_CONCAT,	ASSOC_RIGHT	},
	[TOK_BANG]	 = { PFN(unary),	NULL,			PREC_UNARY,		ASSOC_NONE 	},
	[TOK_GT]	 = { NULL,			PFN(binary),	PREC_COMPARE,	ASSOC_LEFT 	},
	[TOK_LT]	 = { NULL,			PFN(binary),	PREC_COMPARE,	ASSOC_LEFT 	},
	[TOK_GTE]	 = { NULL,			PFN(binary),	PREC_COMPARE,	ASSOC_LEFT 	},
	[TOK_LTE]	 = { NULL,			PFN(binary),	PREC_COMPARE,	ASSOC_LEFT 	},
	[TOK_EQ]	 = { NULL,			PFN(binary),	PREC_COMPARE,	ASSOC_LEFT 	},
	[TOK_NE]	 = { NULL,			PFN(binary),	PREC_COMPARE,	ASSOC_LEFT 	},
	[TOK_AMPAMP] = { NULL,			PFN(binary),	PREC_AND,		ASSOC_LEFT 	},
	[TOK_BARBAR] = { NULL,			PFN(binary),	PREC_OR,		ASSOC_LEFT 	},
	[TOK_ARROW]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_COLCOL] = { NULL,			PFN(binary),	PREC_CONCAT,	ASSOC_RIGHT	},
	[TOK_COMMA]	 = { NULL,			PFN(tuple),		PREC_COMMA,		ASSOC_NONE	},
	[TOK_SEMI]	 = { NULL,			PFN(semi),		PREC_SEMI,		ASSOC_RIGHT },
	[TOK_TILDE]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_INT]	 = { PFN(const),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_FLOAT]	 = { PFN(const),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_STR]	 = { PFN(const),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_CHAR]	 = { PFN(const),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_IDENT]	 = { PFN(ident),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_TRUE]	 = { PFN(const),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_FALSE]	 = { PFN(const),	NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_LET]	 = { PFN(let),		NULL,			PREC_BINDING,	ASSOC_NONE 	},
	[TOK_IN]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_AND]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_FUN]	 = { PFN(fun),		NULL,			PREC_BINDING,	ASSOC_NONE 	},
	[TOK_IF]	 = { PFN(if),		NULL,			PREC_IF,		ASSOC_NONE 	},
	[TOK_THEN]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_ELSE]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_MATCH]	 = { PFN(match),	NULL,			PREC_BINDING,	ASSOC_NONE 	},
	[TOK_WITH]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_TYPE]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
};

static const Primitive prims[] = {
#define TAG(name, id, _)	{ STR_STATIC(#id),	TYPE_ ## name },
#define TYPE
#define BASE
#include "nml_tag.h"
#undef BASE
#undef TYPE
#undef TAG
	{ STR_STATIC(""),	TYPE_NONE },
};

static MAC_COLD void
parser_error_append(Parser *parse, const Sized_Str msg, Error_Flag flags, Token *tok)
{
	if (MAC_UNLIKELY(parse->flags & PARSE_SYNC))
	{
		return;
	}

	parse->flags |= PARSE_MSG;
	error_append(parse->err, parse->arena, msg, tok->loc, flags);
}

static MAC_INLINE inline void
parser_error(Parser *parse, const Sized_Str msg, bool fatal)
{
	parser_error_append(parse, msg, ERR_ERROR, &parse->prev);

	parse->flags |= PARSE_ERR;
	if (fatal)
	{
		parse->flags |= PARSE_SYNC;
	}
}

static MAC_INLINE inline void
parser_warn(Parser *parse, const Sized_Str msg)
{
	parser_error_append(parse, msg, ERR_WARN, &parse->prev);
}

static MAC_HOT void
parser_eat(Parser *parse)
{
	parse->prev = parse->curr;

	if (MAC_UNLIKELY(parse->next.kind != TOK_ERR))
	{
		parse->curr = parse->next;
		parse->next.kind = TOK_ERR;
	}
	else
	{
		lexer_next(&parse->lex, &parse->curr);
	}

	do
	{
		if (parse->curr.kind == TOK_ERR)
		{
			parser_error_append(parse, parse->curr.str, ERR_ERROR, &parse->curr);
			parse->flags |= PARSE_ERR | PARSE_SYNC;
		}
		else
		{
			return;
		}
		lexer_next(&parse->lex, &parse->curr);
	}
	while (true);
}

static MAC_HOT inline void
parser_back(Parser *parse)
{
	if (MAC_LIKELY(parse->next.kind == TOK_ERR))
	{
		parse->next = parse->curr;
		parse->curr = parse->prev;
	}
}

static MAC_HOT inline bool
parser_check(Parser *parse, Token_Kind type)
{
	return parse->curr.kind == type;
}

static MAC_HOT inline bool
parser_match(Parser *parse, Token_Kind type)
{
	if (parser_check(parse, type))
	{
		parser_eat(parse);
		return true;
	}

	return false;
}

static MAC_HOT inline void
parser_expect(Parser *parse, Token_Kind type, const Sized_Str msg)
{
	if (MAC_LIKELY(parser_check(parse, type)))
	{
		parser_eat(parse);
	}
	else
	{
		parser_error(parse, msg, true);
	}
}

static MAC_HOT void
parser_sync(Parser *parse)
{
	if (MAC_UNLIKELY(parse->flags & PARSE_SYNC))
	{
		parse->flags &= ~PARSE_SYNC;

		while (parse->curr.kind != TOK_EOF)
		{
			if ((parse->prev.kind == TOK_SEMI) ||
				(parse->curr.kind >= TOK_LET && parse->curr.kind <= TOK_WITH))
			{
				return;
			}

			parser_eat(parse);
		}
	}
}

static MAC_HOT Type
parser_type_base(Parser *parse)
{
	if (parser_match(parse, TOK_LPAR))
	{
		Type type = parser_type(parse);
		parser_expect(parse, TOK_RPAR, STR_STATIC("Expected `)` after group"));
		return type;
	}
	else
	{
		Sized_Str name = parse->curr.str;
		parser_expect(parse, TOK_IDENT, STR_STATIC("Expected type name"));

		for (size_t i = 0; prims[i].type != TYPE_NONE; ++i)
		{
			if (STR_EQUAL(name, prims[i].name))
			{
				return prims[i].type;
			}
		}

		Type_Ident *ident = arena_alloc(parse->arena, sizeof(Type_Ident));
		ident->name = name;
		return TYPE_TAG(TAG_IDENT, ident);
	}
}

static MAC_HOT Type
parser_type_fun(Parser *parse)
{
	Type type = parser_type_base(parse);

	if (parser_match(parse, TOK_ARROW))
	{
		size_t len = 16;
		Type *pars = arena_lock(parse->arena, len * sizeof(Type));
		pars[0] = type;

		size_t i = 1;
		do
		{
			if (MAC_UNLIKELY(i + 1 == len))
			{
				len += 16;
				pars = arena_update(parse->arena, pars, len * sizeof(Type));
			}
			pars[i++] = parser_type_base(parse);
		}
		while (parser_match(parse, TOK_ARROW));

		Type ret = pars[--i];
		while (i > 0)
		{
			Type_Fun *fun = arena_alloc(parse->arena, sizeof(Type_Fun));
			fun->par = pars[--i];
			fun->ret = ret;
			ret = TYPE_TAG(TAG_FUN, fun);
		}

		arena_reset(parse->arena, pars);
		return ret;
	}

	return type;
}

static MAC_HOT Type
parser_type_tuple(Parser *parse)
{
	Type type = parser_type_fun(parse);

	if (parser_match(parse, TOK_STAR))
	{
		size_t len = 16;
		Type *items = arena_lock(parse->arena, len * sizeof(Type));
		items[0] = type;

		size_t i = 1;
		do
		{
			if (MAC_UNLIKELY(i + 1 == len))
			{
				len += 16;
				items = arena_update(parse->arena, items, len * sizeof(Type));
			}
			items[i++] = parser_type_fun(parse);
		}
		while (parser_match(parse, TOK_STAR));

		items = arena_update(parse->arena, items, i * sizeof(Type));
		arena_unlock(parse->arena, items);

		Type_Tuple *tuple = arena_alloc(parse->arena, sizeof(Type_Tuple));
		tuple->items = items;
		tuple->len = i;
		return TYPE_TAG(TAG_TUPLE, tuple);
	}

	return type;
}

static MAC_INLINE inline Type
parser_type(Parser *parse)
{
	return parser_type_tuple(parse);
}

static MAC_INLINE inline void
parser_const_int(Parser *parse, Const_Value *value)
{
	int64_t res;
	if (parse->prev.str.len > 1 &&
		(parse->prev.str.str[1] == 'x' || parse->prev.str.str[1] == 'X'))
	{
		Sized_Str str = STR_WLEN(parse->prev.str.str + 2, parse->prev.str.len - 2);
		res	= str_to_int(str, 16);
	}
	else
	{
		res = str_to_int(parse->prev.str, 10);
	}

	if (res > INT32_MAX || res < INT32_MIN)
	{
		parser_warn(parse, STR_STATIC("Int literal truncated after overflow"));
		res = res > INT32_MAX ? INT32_MAX : INT32_MIN;
	}

	const int32_t c_int = res;
	const_init(value, CONST_INT, &c_int);
}

static MAC_INLINE inline void
parser_const_float(Parser *parse, Const_Value *value)
{
	const double c_float = str_to_float(parse->prev.str);
	const_init(value, CONST_FLOAT, &c_float);
}

static MAC_HOT void
parser_const_chars(Parser *parse, Const_Value *value)
{
	const char *src = parse->prev.str.str + 1;
	const size_t len = parse->prev.str.len - 1;

	char *buf = arena_alloc(parse->arena, len);
	size_t buflen = 0;

	for (size_t i = 0; i < len; ++i)
	{
		char c = src[i];
		if (MAC_UNLIKELY(c == '\\'))
		{
			switch (src[i + 1])
			{
				case '\\':
				case '"':
				case '\'':
					c = src[++i];
					break;

				case 'n':
					c = '\n';
					++i;
					break;

				case 'r':
					c = '\r';
					++i;
					break;

				case 't':
					c = '\t';
					++i;
					break;

				case 'x':
					if (MAC_UNLIKELY(len < i + 3))
					{
						parser_error(parse, STR_STATIC("Invalid hex escape sequence"), false);
						break;
					}

					c = str_to_int(STR_WLEN(&src[i + 2], 2), 16);
					i += 3;
					break;

				default:
					parser_error(parse, STR_STATIC("Invalid escape sequence"), false);
					break;
			}
		}
		buf[buflen++] = c;
	}

	if (parse->prev.kind == TOK_CHAR)
	{
		const char c_char = buf[0];
		const_init(value, CONST_CHAR, &c_char);
	}
	else
	{
		const Sized_Str c_str = STR_WLEN(buf, buflen);
		const_init(value, CONST_STR, &c_str);
	}
}

static MAC_HOT void
parser_const(Parser *parse, Const_Value *value)
{
	const bool c_true = true;
	const bool c_false = false;

	switch (parse->prev.kind)
	{
		case TOK_INT:
			parser_const_int(parse, value);
			break;

		case TOK_FLOAT:
			parser_const_float(parse, value);
			break;

		case TOK_STR:
		case TOK_CHAR:
			parser_const_chars(parse, value);
			break;

		case TOK_TRUE:
			const_init(value, CONST_BOOL, &c_true);
			break;

		case TOK_FALSE:
			const_init(value, CONST_BOOL, &c_false);
			break;

		default:
			parser_error(parse, STR_STATIC("Expected a constant"), true);
			break;
	}
}

static MAC_HOT void
parser_sig_type(Parser *parse, Sig_Node **node)
{
	Location loc = parse->prev.loc;
	Sized_Str name = parse->curr.str;

	parser_expect(parse, TOK_IDENT, STR_STATIC("Expected identifier"));
	parser_expect(parse, TOK_EQ, STR_STATIC("Expected `=` after identifier"));

	Type type = parser_type(parse);
	*node = sig_type_new(parse->arena, loc, name, type);
}

static MAC_INLINE inline void
parser_sig(Parser *parse, Sig_Node **node)
{
	if (parser_match(parse, TOK_TYPE))
	{
		parser_sig_type(parse, node);
	}
}

static MAC_HOT void
parser_patn_base(Parser *parse, Patn_Node **node)
{
	if (parser_match(parse, TOK_LPAR))
	{
		Location loc = parse->prev.loc;
		if (parser_match(parse, TOK_RPAR))
		{
			*node = patn_const_new(parse->arena, loc, const_new(CONST_UNIT, NULL));
		}
		else if (parse->curr.kind >= TOK_PLUS && parse->curr.kind <= TOK_TILDE)
		{
			Sized_Str ident = parse->curr.str;
			parser_eat(parse);

			parser_expect(parse, TOK_RPAR, STR_STATIC("Expected `)` after identifier"));
			*node = patn_ident_new(parse->arena, loc, ident);
		}
		else
		{
			parser_patn(parse, node);
			parser_expect(parse, TOK_RPAR, STR_STATIC("Expected `)` after group"));
		}
	}
	else if (parser_match(parse, TOK_ULINE) || parser_match(parse, TOK_IDENT))
	{
		*node = patn_ident_new(parse->arena, parse->prev.loc, parse->prev.str);
	}
	else if ((parse->curr.kind >= TOK_IDENT && parse->curr.kind <= TOK_FALSE))
	{
		Location loc = parse->curr.loc;
		parser_eat(parse);

		Const_Value value;
		parser_const(parse, &value);
		*node = patn_const_new(parse->arena, loc, value);
	}
	else
	{
		parser_error(parse, STR_STATIC("Invalid pattern"), false);
	}
}

static MAC_HOT void
parser_patn_tuple(Parser *parse, Patn_Node **node)
{
	parser_patn_base(parse, node);

	if (parser_match(parse, TOK_COMMA))
	{
		size_t len = 8;
		Patn_Node **items = arena_lock(parse->arena, len * sizeof(Patn_Node *));
		items[0] = *node;

		size_t i = 1;
		do
		{
			if (MAC_UNLIKELY(i + 1 == len))
			{
				len += 8;
				items = arena_update(parse->arena, items, len * sizeof(Patn_Node *));
			}
			parser_patn_base(parse, &items[i++]);
		}
		while (parser_match(parse, TOK_COMMA));

		items = arena_update(parse->arena, items, i * sizeof(Patn_Node *));
		arena_unlock(parse->arena, items);
		*node = patn_tuple_new(parse->arena, items[0]->loc, items, i);
	}
}

static MAC_INLINE inline void
parser_patn(Parser *parse, Patn_Node **node)
{
	parser_patn_tuple(parse, node);
}

static MAC_INLINE inline bool
parser_expr_literal(Parser *parse)
{
	return (parse->curr.kind >= TOK_IDENT && parse->curr.kind <= TOK_FALSE) ||
			parser_check(parse, TOK_LPAR) || parser_check(parse, TOK_LBRACK);
}

static MAC_INLINE inline bool
parser_expr_delim(Parser *parse)
{
	return parser_check(parse, TOK_LET) ||
			parser_check(parse, TOK_FUN) ||
			parser_check(parse, TOK_IF) ||
			parser_check(parse, TOK_MATCH);
}

static MAC_HOT void
parser_expr_group(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	if (parser_match(parse, TOK_RPAR))
	{
		*node = expr_const_new(parse->arena, loc, const_new(CONST_UNIT, NULL));
		return;
	}
	else if (parse->curr.kind >= TOK_PLUS && parse->curr.kind <= TOK_TILDE)
	{
		Sized_Str ident = parse->curr.str;
		parser_eat(parse);

		if (parser_match(parse, TOK_RPAR))
		{
			*node = expr_ident_new(parse->arena, loc, ident);
			return;
		}
		parser_back(parse);
	}

	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	parser_expr(parse, node);
	parser_expect(parse, TOK_RPAR, STR_STATIC("Expected `)` after group"));
	parse->scope = prev;
}

static MAC_HOT void
parser_expr_semi(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Expr_Node *expr;
	parser_expr(parse, &expr);
	*node = expr_let_new(parse->arena, loc, STR_STATIC("_"), TYPE_NONE, *node, expr);
}

static MAC_HOT void
parser_expr_uline(Parser *parse, Expr_Node **node)
{
	MAC_UNUSED(node);
	parser_error(parse, STR_STATIC("Unexpected wildcard `_`"), false);
}

static MAC_HOT void
parser_expr_if(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	Expr_Node *cond;
	parser_expr(parse, &cond);

	parser_expect(parse, TOK_THEN, STR_STATIC("Expected `then` after condition"));
	Expr_Node *b_then;
	parser_expr(parse, &b_then);

	parser_expect(parse, TOK_ELSE, STR_STATIC("Expected `else` after then"));
	Expr_Node *b_else;
	parser_expr(parse, &b_else);

	parse->scope = prev;
	*node = expr_if_new(parse->arena, loc, cond, b_then, b_else);
}

static MAC_HOT void
parser_expr_match(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	Expr_Node *value;
	parser_expr(parse, &value);
	parser_expect(parse, TOK_WITH, STR_STATIC("Expected `with` after match value"));

	size_t len = 8;
	struct Expr_Match_Arm *arms = arena_lock(parse->arena, len * sizeof(struct Expr_Match_Arm));
	size_t i = 0;

	Str_Set set;
	str_set_init(&set, 0);

	do
	{
		if (MAC_UNLIKELY(i + 1 == len))
		{
			len += 8;
			arms = arena_update(parse->arena, arms, len * sizeof(struct Expr_Match_Arm));
		}

		parser_patn(parse, &arms[i].patn);
		parser_expect(parse, TOK_ARROW, STR_STATIC("Expected `->` after pattern"));
		parser_expr(parse, &arms[i].expr);
		++i;
	}
	while (parser_match(parse, TOK_BAR));

	arms = arena_update(parse->arena, arms, i * sizeof(struct Expr_Match_Arm));
	arena_unlock(parse->arena, arms);

	parse->scope = prev;
	*node = expr_match_new(parse->arena, loc, value, arms, i);
}

static MAC_HOT void
parser_expr_let(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Sized_Str name = parse->curr.str;
	if (!parser_match(parse, TOK_ULINE))
	{
		parser_expect(parse, TOK_IDENT, STR_STATIC("Expected identifier"));
	}

	Type hint = TYPE_NONE;
	if (parser_match(parse, TOK_COL))
	{
		hint = parser_type(parse);
	}

	parser_expect(parse, TOK_EQ, STR_STATIC("Expected `=` after identifier"));
	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	Expr_Node *value;
	parser_expr(parse, &value);

	Expr_Node *expr = NULL;
	if (MAC_LIKELY(prev == SCOPE_LOCAL) || parser_check(parse, TOK_IN))
	{
		parser_expect(parse, TOK_IN, STR_STATIC("Expected `in` after expression"));
		parser_expr(parse, &expr);
	}

	parse->scope = prev;
	*node = expr_let_new(parse->arena, loc, name, hint, value, expr);
}

static MAC_HOT void
parser_expr_fun(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	size_t len = 8;
	Sized_Str *pars = arena_lock(parse->arena, len * sizeof(Sized_Str));

	if (parser_match(parse, TOK_IDENT) || parser_match(parse, TOK_ULINE))
	{
		*pars = parse->prev.str;
	}
	else
	{
		parser_error(parse, STR_STATIC("Expected `fun` parameters"), true);
	}

	size_t i = 1;
	while (parser_match(parse, TOK_IDENT) || parser_match(parse, TOK_ULINE))
	{
		if (MAC_UNLIKELY(i + 1 == len))
		{
			len += 8;
			pars = arena_update(parse->arena, pars, len * sizeof(Sized_Str));
		}
		pars[i++] = parse->prev.str;
	}

	len = i;
	pars = arena_update(parse->arena, pars, len * sizeof(Sized_Str));
	arena_unlock(parse->arena, pars);

	Expr_Node *expr = NULL;
	bool anon = false;
	Sized_Str name;

	Type hint = TYPE_NONE;
	if (parser_match(parse, TOK_ARROW))
	{
		// TODO: Consider adding type hints for anonymous function
		name = STR_STATIC("@anon");
		anon = true;
	}
	else
	{
		if (parser_match(parse, TOK_COL))
		{
			hint = parser_type_fun(parse);
		}

		parser_expect(parse, TOK_EQ, STR_STATIC("Expected `=` after parameters"));

		if  (--len == 0)
		{
			parser_error(parse, STR_STATIC("Expected `fun` parameters"), false);
		}
		name = *pars++;
	}

	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	Expr_Node *body;
	parser_expr(parse, &body);

	if (!anon)
	{
		if (prev == SCOPE_LOCAL || parser_check(parse, TOK_IN))
		{
			parser_expect(parse, TOK_IN, STR_STATIC("Expected `in` after function"));
			parser_expr(parse, &expr);
		}
	}

	parse->scope = prev;
	*node = expr_fun_new(parse->arena, loc, name, hint, pars, len, body, expr);
}

static MAC_HOT void
parser_expr_app(Parser *parse, Expr_Node **node)
{
	if (parser_expr_literal(parse))
	{
		size_t len = 8;
		Expr_Node **args = arena_lock(parse->arena, len * sizeof(Expr_Node *));
		size_t i = 0;

		do
		{
			if (MAC_UNLIKELY(i + 1 == len))
			{
				len += 8;
				args = arena_update(parse->arena, args, len * sizeof(Expr_Node *));
			}
			parser_expr_prec(parse, &args[i++], PREC_APP + 1);
		}
		while (parser_expr_literal(parse));

		arena_update(parse->arena, args, i * sizeof(Expr_Node *));
		arena_unlock(parse->arena, args);
		*node = expr_app_new(parse->arena, (*node)->loc, *node, args, i);
	}
}

static MAC_INLINE inline void
parser_expr_ident(Parser *parse, Expr_Node **node)
{
	*node = expr_ident_new(parse->arena, parse->prev.loc, parse->prev.str);
}

static MAC_HOT void
parser_expr_unary(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Token_Kind op = parse->prev.kind;

	Expr_Node *lhs;
	parser_expr_prec(parse, &lhs, PREC_UNARY);
	*node = expr_unary_new(parse->arena, loc, op, lhs);
}

static MAC_HOT void
parser_expr_binary(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Token_Kind op = parse->prev.kind;

	Expr_Node *lhs;
	parser_expr_prec(parse, &lhs, precs[op].prec + precs[op].assoc);
	*node = expr_binary_new(parse->arena, loc, op, *node, lhs);
}

static MAC_HOT void
parser_expr_tuple(Parser *parse, Expr_Node **node)
{
	if ((*node)->kind == EXPR_TUPLE)
	{
		Expr_Tuple *tuple = (Expr_Tuple *)*node;
		size_t len = tuple->len + 1;
		arena_update(parse->arena, tuple->items, len * sizeof(Expr_Node *));
		parser_expr_prec(parse, &tuple->items[tuple->len++], PREC_COMMA + 1);
	}
	else
	{
		Expr_Node **items = arena_alloc(parse->arena, 8 * sizeof(Expr_Node *));
		items[0] = *node;
		parser_expr_prec(parse, items + 1, PREC_COMMA + 1);
		*node = expr_tuple_new(parse->arena, items[0]->loc, items, 2);
	}

	if (!parser_check(parse, TOK_COMMA))
	{
		Expr_Tuple *tuple = (Expr_Tuple *)*node;
		arena_unlock(parse->arena, tuple->items);
	}
}

static MAC_HOT void
parser_expr_const(Parser *parse, Expr_Node **node)
{
	Location loc = parse->prev.loc;
	Const_Value value;
	parser_const(parse, &value);
	*node = expr_const_new(parse->arena, loc, value);
}

static MAC_HOT void
parser_expr_prec(Parser *parse, Expr_Node **node, Parser_Prec prec)
{
	parser_eat(parse);
	Parser_Fn prefix = precs[parse->prev.kind].prefix;

	if (prefix == NULL)
	{
		parser_error(parse, STR_STATIC("Unexpected token"), true);
		return;
	}

	Parser_Prec prev = parse->prec;
	parse->prec = prec;
	prefix(parse, node);

	while (prec <= precs[parse->curr.kind].prec &&
			MAC_LIKELY(!parser_expr_delim(parse)))
	{
		parser_eat(parse);
		precs[parse->prev.kind].infix(parse, node);
	}

	if (prec < PREC_APP)
	{
		parser_expr_app(parse, node);

		while (prec <= precs[parse->curr.kind].prec &&
				MAC_LIKELY(!parser_expr_delim(parse)))
		{
			parser_eat(parse);
			precs[parse->prev.kind].infix(parse, node);
		}
	}

	parse->prec = prev;
}

static MAC_INLINE inline void
parser_expr(Parser *parse, Expr_Node **node)
{
	parser_expr_prec(parse, node, PREC_BINDING);
	parser_sync(parse);
}

static MAC_INLINE inline void
parser_top(Parser *parse, Ast_Node *node)
{
	// FIXME: Implement proper signature check
	if (parser_check(parse, TOK_TYPE))
	{
		node->sig = true;
		parser_sig(parse, (Sig_Node **)&node->node);
		parser_sync(parse);
	}
	else
	{
		node->sig = false;
		parse->scope = SCOPE_TOP;
		parser_expr(parse, (Expr_Node **)&node->node);
	}
}

void
parser_init(Parser *parse, Arena *arena, const Sized_Str src)
{
	lexer_init(&parse->lex, src);
	parse->next.kind = TOK_ERR;

	parse->arena = arena;
	parse->flags = PARSE_OK;
	parse->err = NULL;
	parse->prec = PREC_NONE;
	parse->scope = SCOPE_NONE;
}

void
parser_reset(Parser *parse)
{
	lexer_reset(&parse->lex);
	parse->next.kind = TOK_ERR;

	parse->flags = PARSE_OK;
	parse->err = NULL;
	parse->prec = PREC_NONE;
	parse->scope = SCOPE_NONE;
}

MAC_HOT bool
parser_parse(Parser *parse, Ast_Top *ast, Error_List *err)
{
	parse->err = err;

	size_t len = 16;
	Ast_Node *items = arena_lock(parse->arena, len * sizeof(Ast_Node));
	size_t i = 0;

	parser_eat(parse);
	while (!parser_match(parse, TOK_EOF))
	{
		if (MAC_UNLIKELY(i + 1 == len))
		{
			len += 16;
			items = arena_update(parse->arena, items, len * sizeof(Ast_Node));
		}
		parser_top(parse, &items[i++]);
	}

	items = arena_update(parse->arena, items, i * sizeof(Ast_Node));
	arena_unlock(parse->arena, items);

	if (parse->flags & PARSE_ERR)
	{
		ast->items = NULL;
		ast->len = 0;
		return false;
	}
	else
	{
		ast->items = items;
		ast->len = i;
		return true;
	}
}
