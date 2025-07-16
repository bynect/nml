#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nml_ast.h"
#include "nml_mac.h"
#include "nml_err.h"
#include "nml_lex.h"
#include "nml_mem.h"
#include "nml_parse.h"
#include "nml_str.h"
#include "nml_type.h"
#include "nml_fmt.h"

#define PFN(name) (Parser_Fn)parser_ ## name

typedef void (*const Parser_Fn)(Parser *parse, Ast_Node **node);

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

static MAC_HOT void parser_eat(Parser *parse);
static MAC_HOT inline void parser_back(Parser *parse);
static MAC_HOT inline bool parser_check(Parser *parse, Token_Kind type);
static MAC_HOT inline bool parser_match(Parser *parse, Token_Kind type);
static MAC_HOT inline void parser_expect(Parser *parse, Token_Kind type, const Sized_Str msg);

static MAC_HOT void parser_group(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_semi(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_uline(Parser *parse, Ast_Node **node);

static MAC_HOT void parser_if(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_let(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_fun(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_app(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_ident(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_unary(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_binary(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_tuple(Parser *parse, Ast_Node **node);
static MAC_HOT void parser_const(Parser *parse, Ast_Node **node);

static MAC_HOT void parser_prec(Parser *parse, Ast_Node **node, Parser_Prec prec);
static MAC_HOT void parser_expr(Parser *parse, Ast_Node **node);

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
	[TOK_MATCH]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_WITH]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
	[TOK_TYPE]	 = { NULL,			NULL,			PREC_NONE,		ASSOC_NONE 	},
};

static const Primitive prims[] = {
#define TAG(name, str, _)	{ STR_STATIC(#str),	TYPE_ ## name },
#define TYPE
#define BASE
#include "nml_tag.h"
#undef BASE
#undef TYPE
#undef TAG
	{ STR_STATIC(""),	TYPE_NONE },
};

static MAC_INLINE inline void parser_error(Parser *parse, const Sized_Str msg);
static MAC_INLINE inline Type parser_type(Parser *parse);

static MAC_HOT Type
parser_type_id(Parser *parse)
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

		Sized_Str msg = format_str(parse->arena,
									STR_STATIC("Unbound type name `{S}`"), name);
		parser_error(parse, msg);
		return TYPE_NONE;
	}
}

static MAC_HOT Type
parser_type_fun(Parser *parse)
{
	Type type = parser_type_id(parse);

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
			pars[i++] = parser_type_id(parse);
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

static MAC_COLD void
parser_error_append(Parser *parse, const Sized_Str msg, Error_Flag flags, Token *tok)
{
	if (MAC_UNLIKELY(parse->flags & PARSE_SYNC))
	{
		return;
	}

	parse->flags |= PARSE_MSG;
	error_append(parse->err, parse->arena, msg, tok->line, flags);
}

static MAC_INLINE inline void
parser_error(Parser *parse, const Sized_Str msg)
{
	parser_error_append(parse, msg, ERR_ERROR, &parse->prev);
	parse->flags |= PARSE_ERR | PARSE_SYNC;
}

static MAC_INLINE inline void
parser_warn(Parser *parse, const Sized_Str msg)
{
	parser_error_append(parse, msg, ERR_WARN, &parse->prev);
}

static MAC_INLINE inline void
parser_info(Parser *parse, const Sized_Str msg)
{
	parser_error_append(parse, msg, ERR_INFO, &parse->prev);
}

static MAC_HOT void
parser_eat(Parser *parse)
{
	parse->prev = parse->curr;

	if (MAC_UNLIKELY(parse->next.line != 0))
	{
		parse->curr = parse->next;
		parse->next.line = 0;
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
	if (MAC_LIKELY(parse->next.line == 0))
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
	if (parser_check(parse, type))
	{
		parser_eat(parse);
	}
	else
	{
		parser_error(parse, msg);
	}
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
parser_group(Parser *parse, Ast_Node **node)
{
	if (parser_match(parse, TOK_RPAR))
	{
		*node = ast_const_new(parse->arena, CONST_UNIT, NULL);
		return;
	}
	else if (parse->curr.kind >= TOK_PLUS && parse->curr.kind <= TOK_TILDE)
	{
		Sized_Str ident = parse->curr.str;
		parser_eat(parse);

		if (parser_match(parse, TOK_RPAR))
		{
			*node = ast_ident_new(parse->arena, ident);
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
parser_semi(Parser *parse, Ast_Node **node)
{
	Ast_Node *expr;
	parser_expr(parse, &expr);
	*node = ast_let_new(parse->arena, STR_STATIC("_"), TYPE_NONE, *node, expr);
}

static MAC_HOT void
parser_uline(Parser *parse, Ast_Node **node)
{
	MAC_UNUSED(node);
	parser_error(parse, STR_STATIC("Unexpected wildcard `_`"));
}

static MAC_HOT void
parser_if(Parser *parse, Ast_Node **node)
{
	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	Ast_Node *cond;
	parser_expr(parse, &cond);

	parser_expect(parse, TOK_THEN, STR_STATIC("Expected `then` after condition"));
	Ast_Node *then;
	parser_expr(parse, &then);

	Ast_Node *m_else = NULL;
	if (parser_match(parse, TOK_ELSE))
	{
		parser_expr(parse, &m_else);
	}

	parse->scope = prev;
	*node = ast_if_new(parse->arena, cond, then, m_else);
}

static MAC_HOT void
parser_let(Parser *parse, Ast_Node **node)
{
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

	Ast_Node *value;
	parser_expr(parse, &value);

	Ast_Node *expr = NULL;
	if (MAC_LIKELY(prev == SCOPE_LOCAL) || parser_check(parse, TOK_IN))
	{
		parser_expect(parse, TOK_IN, STR_STATIC("Expected `in` after expression"));
		parser_expr(parse, &expr);
	}

	parse->scope = prev;
	*node = ast_let_new(parse->arena, name, hint, value, expr);
}

static MAC_HOT void
parser_fun(Parser *parse, Ast_Node **node)
{
	size_t len = 8;
	Sized_Str *pars = arena_lock(parse->arena, len * sizeof(Sized_Str));

	if (parser_match(parse, TOK_IDENT) || parser_match(parse, TOK_ULINE))
	{
		*pars = parse->prev.str;
	}
	else
	{
		parser_error(parse, STR_STATIC("Expected `fun` parameters"));
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

	Ast_Node *expr = NULL;
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
			parser_error(parse, STR_STATIC("Expected `fun` parameters"));
		}
		name = *pars++;
	}

	Parser_Scope prev = parse->scope;
	parse->scope = SCOPE_LOCAL;

	Ast_Node *body;
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
	*node = ast_fun_new(parse->arena, name, hint, pars, len, body, expr);
}

static MAC_HOT void
parser_app(Parser *parse, Ast_Node **node)
{
	if (parser_expr_literal(parse))
	{
		size_t len = 8;
		Ast_Node **args = arena_lock(parse->arena, len * sizeof(Ast_Node *));
		size_t i = 0;

		do
		{
			if (MAC_UNLIKELY(i + 1 == len))
			{
				len += 8;
				args = arena_update(parse->arena, args, len * sizeof(Ast_Node *));
			}
			parser_prec(parse, &args[i++], PREC_APP + 1);
		}
		while (parser_expr_literal(parse));

		arena_update(parse->arena, args, i * sizeof(Ast_Node *));
		arena_unlock(parse->arena, args);
		*node = ast_app_new(parse->arena, *node, args, i);
	}
}

static MAC_INLINE inline void
parser_ident(Parser *parse, Ast_Node **node)
{
	*node = ast_ident_new(parse->arena, parse->prev.str);
}

static MAC_HOT void
parser_unary(Parser *parse, Ast_Node **node)
{
	Token_Kind op = parse->prev.kind;
	Ast_Node *lhs;
	parser_prec(parse, &lhs, PREC_UNARY);
	*node = ast_unary_new(parse->arena, op, lhs);
}

static MAC_HOT void
parser_binary(Parser *parse, Ast_Node **node)
{
	Token_Kind op = parse->prev.kind;
	Ast_Node *lhs;
	parser_prec(parse, &lhs, precs[op].prec + precs[op].assoc);
	*node = ast_binary_new(parse->arena, op, *node, lhs);
}

static MAC_HOT void
parser_tuple(Parser *parse, Ast_Node **node)
{
	if ((*node)->kind == AST_TUPLE)
	{
		Ast_Tuple *tuple = (Ast_Tuple *)*node;
		size_t len = tuple->len + 1;
		arena_update(parse->arena, tuple->items, len * sizeof(Ast_Node *));
		parser_prec(parse, &tuple->items[tuple->len++], PREC_COMMA + 1);
	}
	else
	{
		Ast_Node **items = arena_alloc(parse->arena, 8 * sizeof(Ast_Node *));
		items[0] = *node;
		parser_prec(parse, items + 1, PREC_COMMA + 1);
		*node = ast_tuple_new(parse->arena, items, 2);
	}

	if (!parser_check(parse, TOK_COMMA))
	{
		Ast_Tuple *tuple = (Ast_Tuple *)*node;
		arena_unlock(parse->arena, tuple->items);
	}
}

static MAC_INLINE inline void
parser_int(Parser *parse, Ast_Node **node)
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
	*node = ast_const_new(parse->arena, CONST_INT, &c_int);
}

static MAC_INLINE inline void
parser_float(Parser *parse, Ast_Node **node)
{
	const double c_float = str_to_float(parse->prev.str);
	*node = ast_const_new(parse->arena, CONST_FLOAT, &c_float);
}

static MAC_HOT void
parser_chars(Parser *parse, Ast_Node **node)
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
						parser_error(parse, STR_STATIC("Invalid hex escape sequence"));
						break;
					}

					const char hex[3] = {
						src[ i+ 2], src[i + 3], '\0',
					};
					c = strtol(hex, NULL, 16);
					i += 3;
					break;

				default:
					parser_error(parse, STR_STATIC("Invalid escape sequence"));
					break;
			}
		}
		buf[buflen++] = c;
	}

	if (parse->prev.kind == TOK_CHAR)
	{
		const char c_char = buf[0];
		*node = ast_const_new(parse->arena, CONST_CHAR, &c_char);
	}
	else
	{
		const Sized_Str c_str = STR_WLEN(buf, buflen);
		*node = ast_const_new(parse->arena, CONST_STR, &c_str);
	}
}

static MAC_HOT void
parser_const(Parser *parse, Ast_Node **node)
{
	const bool c_true = true;
	const bool c_false = false;

	switch (parse->prev.kind)
	{
		case TOK_INT:
			parser_int(parse, node);
			break;

		case TOK_FLOAT:
			parser_float(parse, node);
			break;

		case TOK_STR:
		case TOK_CHAR:
			parser_chars(parse, node);
			break;

		case TOK_TRUE:
			*node = ast_const_new(parse->arena, CONST_BOOL, &c_true);
			break;

		case TOK_FALSE:
			*node = ast_const_new(parse->arena, CONST_BOOL, &c_false);
			break;

		default:
			parser_error(parse, STR_STATIC("Not a constant"));
			break;
	}
}

static MAC_HOT void
parser_prec(Parser *parse, Ast_Node **node, Parser_Prec prec)
{
	parser_eat(parse);
	Parser_Fn prefix = precs[parse->prev.kind].prefix;

	if (prefix == NULL)
	{
		parser_error(parse, STR_STATIC("Unexpected token"));
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
		parser_app(parse, node);

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
parser_expr(Parser *parse, Ast_Node **node)
{
	parser_prec(parse, node, PREC_BINDING);
}

static MAC_INLINE inline void
parser_top(Parser *parse, Ast_Node **node)
{
	parse->scope = SCOPE_TOP;
	parser_expr(parse, node);
}

void
parser_init(Parser *parse, Arena *arena, const Sized_Str src)
{
	lexer_init(&parse->lex, src);
	parse->next.line = 0;

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
	parse->next.line = 0;

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
	Ast_Node **items = arena_lock(parse->arena, len * sizeof(Ast_Node *));
	size_t i = 0;

	parser_eat(parse);
	while (!parser_match(parse, TOK_EOF))
	{
		if (MAC_UNLIKELY(i + 1 == len))
		{
			len += 16;
			items = arena_update(parse->arena, items, len * sizeof(Ast_Node *));
		}
		parser_top(parse, &items[i++]);
	}

	items = arena_update(parse->arena, items, i * sizeof(Ast_Node *));
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
