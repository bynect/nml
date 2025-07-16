#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nml_dbg.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_str.h"

typedef struct {
	const Sized_Str name;
	const Token_Kind kind;
} Keyword;

static const Keyword kwds[] = {
#define TOK(kind, _, str, type)	{ STR_STATIC(#str),	TOK_ ## kind },
#define KWD
#include "nml_tok.h"
#undef KWD
#undef TOK
	{ STR_STATIC("_"),	TOK_ULINE },
	{ STR_STATIC(""),	TOK_EOF },
};

static MAC_COLD inline void
lexer_err(Lexer *lex, Token *tok, const Sized_Str msg)
{
	tok->kind = TOK_ERR;
	tok->loc = lex->loc;
	tok->str = msg;
}

static MAC_INLINE inline void
lexer_tok(Lexer *lex, Token *tok, Token_Kind kind)
{
	tok->kind = kind;
	tok->loc = lex->loc;

	size_t len = lex->off - lex->soff;
	tok->loc.col -= len;
	tok->str = STR_WLEN(lex->src.str + lex->soff, len);
}

static MAC_INLINE inline bool
lexer_eof(Lexer *lex)
{
	return lex->off >= lex->src.len;
}

static MAC_INLINE inline void
lexer_line(Lexer *lex)
{
	++lex->loc.line;
	lex->loc.col = 1;
}

static MAC_INLINE inline char
lexer_peek(Lexer *lex)
{
	return lexer_eof(lex) ? '\0' : lex->src.str[lex->off];
}

static MAC_INLINE inline char
lexer_peektwo(Lexer *lex)
{
	if (lex->off + 1 >= lex->src.len)
	{
		return '\0';
	}
	return lex->src.str[lex->off + 1];
}

static MAC_INLINE inline void
lexer_eat(Lexer *lex)
{
	++lex->off;
	++lex->loc.col;
}

static MAC_INLINE inline char
lexer_curr(Lexer *lex)
{
	++lex->loc.col;
	return lex->src.str[lex->off++];
}

static MAC_INLINE inline bool
lexer_match(Lexer *lex, char c)
{
	if (lexer_eof(lex) || lex->src.str[lex->off] != c)
	{
		return false;
	}

	lexer_eat(lex);
	return true;
}

static MAC_INLINE inline bool
lexer_comment(Lexer *lex, Token *tok)
{
	lexer_eat(lex);
	lexer_eat(lex);

	uint16_t depth = 1;
	while (depth > 0)
	{
		if (lexer_eof(lex))
		{
			lexer_err(lex, tok, STR_STATIC("Unterminated comment"));
			return true;
		}

		char c = lexer_peek(lex);
		if (c == '(' && lexer_peektwo(lex) == '*')
		{
			++depth;
			lexer_eat(lex);
		}
		else if (c == '*' && lexer_peektwo(lex) == ')')
		{
			--depth;
			lexer_eat(lex);
		}
		else if (c == '\n')
		{
			lexer_line(lex);
		}

		lexer_eat(lex);
	}

	// TODO: Fix comment tokenization
	// Comment tokenization and handling is required for
	// pretty printing.
	// The implementation a formatter is delayed since it
	// needs special ast nodes and there are more urgent matters.
	return false;
}

static inline bool
lexer_skip(Lexer *lex, Token *tok)
{
	while (true)
	{
		switch (lexer_peek(lex))
		{
			case '\n':
				lexer_line(lex);
				lexer_eat(lex);
				break;

			case ' ':
			case '\t':
			case '\r':
				lexer_eat(lex);
				break;

			case '(':
				if (lexer_peektwo(lex) == '*')
				{
					if (lexer_comment(lex, tok))
					{
						return true;
					}
					else
					{
						break;
					}
				}
				else
				{
					return false;
				}

			default:
				return false;
		}
	}
}

static MAC_HOT void
lexer_ident(Lexer *lex, Token *tok)
{
	while (isalnum(lexer_peek(lex)) || lexer_peek(lex) == '_')
	{
		lexer_eat(lex);
	}

	Token_Kind kind = TOK_IDENT;
	size_t len = lex->off - lex->soff;

	for (size_t i = 0; kwds[i].kind != TOK_EOF; ++i)
	{
		if (len == kwds[i].name.len &&
			!memcmp(lex->src.str + lex->soff, kwds[i].name.str, len))
		{
			kind = kwds[i].kind;
			break;
		}
	}

	lexer_tok(lex, tok, kind);
}

static MAC_HOT void
lexer_num(Lexer *lex, Token *tok)
{
	Token_Kind kind = TOK_INT;
	if (lexer_peek(lex) == 'x' || lexer_peek(lex) == 'X')
	{
		do
		{
			lexer_eat(lex);
		}
		while (isxdigit(lexer_peek(lex)));
	}
	else
	{
		while (isdigit(lexer_peek(lex)))
		{
			lexer_eat(lex);
		}

		if (lexer_peek(lex) == '.' && isdigit(lexer_peektwo(lex)))
		{
			kind = TOK_FLOAT;
			do
			{
				lexer_eat(lex);
			}
			while (isdigit(lexer_peek(lex)));
		}

		if (lexer_match(lex, 'e') || lexer_match(lex, 'E'))
		{
			kind = TOK_FLOAT;
			if (lexer_peek(lex) == '+' || lexer_peek(lex) == '-')
			{
				lexer_eat(lex);
			}

			if (isdigit(lexer_peek(lex)))
			{
				do
				{
					lexer_eat(lex);
				}
				while (isdigit(lexer_peek(lex)));
			}
			else
			{
				lexer_err(lex, tok, STR_STATIC("Expected exponent digits"));
				return;
			}
		}
	}

	if (isalnum(lexer_peek(lex)) || lexer_peek(lex) == '_')
	{
		lexer_err(lex, tok, STR_STATIC("Unexpected characters after literal"));
		return;
	}
	lexer_tok(lex, tok, kind);
}

static MAC_HOT void
lexer_str(Lexer *lex, Token *tok)
{
	while (lexer_peek(lex) != '"' && !lexer_eof(lex))
	{
		if (MAC_UNLIKELY(lexer_peek(lex) == '\n'))
		{
			lexer_line(lex);
		}
		else if (MAC_UNLIKELY(lexer_peek(lex) == '\\' && lexer_peektwo(lex) == '"'))
		{
			lexer_eat(lex);
		}
		lexer_eat(lex);
	}

	if (lexer_eof(lex))
	{
		lexer_err(lex, tok, STR_STATIC("Unterminated string literal"));
	}
	else
	{
		lexer_eat(lex);
		lexer_tok(lex, tok, TOK_STR);
	}
}

static MAC_HOT void
lexer_char(Lexer *lex, Token *tok)
{
	if (lexer_eof(lex) || lexer_peek(lex) == '\n')
	{
		lexer_err(lex, tok, STR_STATIC("Unterminated char literal"));
		return;
	}

	char c = lexer_curr(lex);
	if (c == '\'')
	{
		lexer_err(lex, tok, STR_STATIC("Unexpected empty char literal"));
		return;
	}
	else if (c == '\\')
	{
		switch (lexer_peek(lex))
		{
			case '\\':
			case '"':
			case '\'':
			case 'n':
			case 'r':
			case 't':
				break;

			case 'x':
				lexer_eat(lex);
				if (lexer_peek(lex) == '\'' || lexer_peektwo(lex) == '\'')
				{
					if (lexer_peektwo(lex) == '\'')
					{
						lexer_eat(lex);
					}
					lexer_eat(lex);

					lexer_err(lex, tok, STR_STATIC("Unterminated hex escape sequence"));
					return;
				}
				else if (!isxdigit(lexer_peek(lex)) || !isxdigit(lexer_peektwo(lex)))
				{
					lexer_err(lex, tok, STR_STATIC("Unexpected character in hex escape sequence"));
					return;
				}
				lexer_eat(lex);
				break;

			default:
				lexer_err(lex, tok, STR_STATIC("Unexpected character in escape sequence"));
				return;
		}
		lexer_eat(lex);
	}

	if (lexer_peek(lex) != '\'')
	{
		while (lexer_peek(lex) != '\'' && !lexer_eof(lex))
		{
			if (lexer_peek(lex) == '\n')
			{
				lexer_line(lex);
			}
			lexer_eat(lex);
		}

		if (lexer_eof(lex))
		{
			lexer_err(lex, tok, STR_STATIC("Unterminated char literal"));
		}
		else
		{
			lexer_eat(lex);
			lexer_err(lex, tok, STR_STATIC("Unexpected characters in char literal"));
		}
	}
	else
	{
		lexer_eat(lex);
		lexer_tok(lex, tok, TOK_CHAR);
	}
}

void
lexer_init(Lexer *lex, const Source *src)
{
	lex->loc = (Location) {1, 1, src};
	lex->src = src->src;
	lex->off = 0;
	lex->soff = 0;
}

void
lexer_reset(Lexer *lex)
{
	lex->loc.line = 1;
	lex->loc.col = 1;
	lex->off = 0;
	lex->soff = 0;
}

MAC_HOT bool
lexer_next(Lexer *lex, Token *tok)
{
	if (lexer_skip(lex, tok))
	{
		return true;
	}

	if (lexer_eof(lex))
	{
		lexer_tok(lex, tok, TOK_EOF);
		return false;
	}

	lex->soff = lex->off;
	char c = lexer_curr(lex);

	if (isalpha(c) || c == '_')
	{
		lexer_ident(lex, tok);
		return true;
	}

	Token_Kind kind;
	switch (c)
	{
		case '(':
			kind = TOK_LPAR;
			break;

		case ')':
			kind = TOK_RPAR;
			break;

		case '[':
			kind = TOK_LBRACK;
			break;

		case ']':
			kind = TOK_RBRACK;
			break;

		case '{':
			kind = TOK_LBRACE;
			break;

		case '}':
			kind = TOK_RBRACE;
			break;

		case '+':
			kind = lexer_match(lex, '.') ? TOK_PLUSF : TOK_PLUS;
			break;

		case '-':
			kind = lexer_match(lex, '>') ? TOK_ARROW :
					lexer_match(lex, '.') ? TOK_MINUSF : TOK_MINUS;

			if (kind == TOK_MINUS && isdigit(lexer_peek(lex)))
			{
				lexer_num(lex, tok);
				return true;
			}
			break;

		case '*':
			kind = lexer_match(lex, '.') ? TOK_STARF : TOK_STAR;
			break;

		case '/':
			kind = lexer_match(lex, '.') ? TOK_SLASHF : TOK_SLASH;
			break;

		case '%':
			kind = lexer_match(lex, '.') ? TOK_PERCF : TOK_PERC;
			break;

		case '^':
			kind = TOK_CARET;
			break;

		case '.':
			kind = TOK_DOT;
			break;

		case '@':
			kind = TOK_AT;
			break;

		case '#':
			kind = TOK_HASH;
			break;

		case ':':
			kind = lexer_match(lex, ':') ? TOK_COLCOL : TOK_COL;
			break;

		case '!':
			kind = lexer_match(lex, '=') ? TOK_NE : TOK_BANG;
			break;

		case '>':
			kind = lexer_match(lex, '=') ? TOK_GTE : TOK_GT;
			break;

		case '<':
			kind = lexer_match(lex, '=') ? TOK_LTE : TOK_LT;
			break;

		case '=':
			kind = TOK_EQ;
			break;

		case '&':
			kind = lexer_match(lex, '&') ? TOK_AMPAMP : TOK_AMP;
			break;

		case '|':
			kind = lexer_match(lex, '|') ? TOK_BARBAR : TOK_BAR;
			break;

		case '`':
			kind = TOK_BTICK;
			break;

		case ',':
			kind = TOK_COMMA;
			break;

		case ';':
			kind = TOK_SEMI;
			break;

		case '$':
			kind = TOK_DOLLAR;
			break;

		case '~':
			kind = TOK_TILDE;
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			lexer_num(lex, tok);
			return true;

		case '"':
			lexer_str(lex, tok);
			return true;

		case '\'':
			lexer_char(lex, tok);
			return true;

		default:
			lexer_err(lex, tok, STR_STATIC("Unexpected character"));
			return true;
	}

	lexer_tok(lex, tok, kind);
	return true;
}
