#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

typedef struct {
	const Token_Type type;
	const char *const name;
	const size_t len;
} Keyword;

static const Keyword keywords[] = {
	{ TOK_TRUE, "true", 4 },
	{ TOK_FALSE, "false", 5 },
	{ TOK_LET, "let", 3 },
	{ TOK_FUN, "fun", 3 },
	{ TOK_EOF, NULL, 0 },
};

static inline void
lexer_tok(Lexer *lex, Token *tok, Token_Type type)
{
	tok->type = type;
	tok->line = lex->line;
	tok->src = lex->src + lex->soff;
	tok->len = lex->off - lex->soff;
}

static inline void
lexer_err(Lexer *lex, Token *tok, const char *const msg)
{
	tok->type = TOK_ERR;
	tok->line = lex->line;
	tok->src = msg;
	tok->len = strlen(msg);
}

static inline bool
lexer_eof(Lexer *lex)
{
	return lex->off >= lex->len;
}

static inline char
lexer_peek(Lexer *lex)
{
	return lexer_eof(lex) ? '\0' : lex->src[lex->off];
}

static inline char
lexer_peektwo(Lexer *lex)
{
	if (lex->off + 1 >= lex->len)
	{
		return '\0';
	}
	return lex->src[lex->off + 1];
}

static inline bool
lexer_match(Lexer *lex, char c)
{
	if (lexer_eof(lex) || lex->src[lex->off] != c)
	{
		return false;
	}

	++lex->off;
	return true;
}

static inline char
lexer_eat(Lexer *lex)
{
	return lex->src[lex->off++];
}

static inline bool
lexer_comment(Lexer *lex, Token *tok)
{
	lexer_eat(lex);
	lexer_eat(lex);

	uint16_t depth = 1;
	while (depth > 0)
	{
		if (lexer_eof(lex))
		{
			lexer_err(lex, tok, "Unterminated comment");
			return false;
		}

		char c = lexer_peek(lex);
		char c2 = lexer_peektwo(lex);

		if (c == '(' && c2 == '*')
		{
			++depth;
			lexer_eat(lex);
		}
		else if (c == '*' && c2 == ')')
		{
			--depth;
			lexer_eat(lex);
		}

		lexer_eat(lex);
	}
	return true;
}

static inline bool
lexer_skip(Lexer *lex, Token *tok)
{
	while (true)
	{
		switch (lexer_peek(lex))
		{
			case '\n':
				++lex->line;
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
					if (!lexer_comment(lex, tok))
					{
						return false;
					}
					break;
				}
				else
				{
					return true;
				}

			default:
				return true;
		}
	}
}

static void
lexer_ident(Lexer *lex, Token *tok)
{
	while (isalnum(lexer_peek(lex)))
	{
		lexer_eat(lex);
	}

	Token_Type type = TOK_IDENT;
	size_t len = lex->off - lex->soff;

	for (uint8_t i = 0; keywords[i].name != NULL; ++i)
	{
		if (len == keywords[i].len &&
			!strncmp(lex->src + lex->soff, keywords[i].name, len))
		{
			type = keywords[i].type;
			break;
		}
	}

	lexer_tok(lex, tok, type);
}

static void
lexer_num(Lexer *lex, Token *tok)
{
	Token_Type type = TOK_INT;
	while (isdigit(lexer_peek(lex)))
	{
		lexer_eat(lex);
	}

	if (lexer_peek(lex) == '.' && isdigit(lexer_peektwo(lex)))
	{
		lexer_eat(lex);
		type = TOK_FLOAT;

		while (isdigit(lexer_peek(lex)))
		{
			lexer_eat(lex);
		}
	}

	lexer_tok(lex, tok, type);
}

static void
lexer_str(Lexer *lex, Token *tok)
{
	while (lexer_peek(lex) != '"' && !lexer_eof(lex))
	{
		lexer_peek(lex) == '\n' && ++lex->line;
		lexer_eat(lex);
	}

	if (lexer_eof(lex))
	{
		lexer_err(lex, tok, "Unterminated string");
	}
	else
	{
		lexer_eat(lex);
		lexer_tok(lex, tok, TOK_STR);
	}
}

static void
lexer_char(Lexer *lex, Token *tok)
{
	lexer_err(lex, tok, "TODO");
}

Lexer
lexer_init(const char *const src, const size_t len)
{
	return (Lexer) {
		.line = 1,
		.src = src,
		.off = 0,
		.soff = 0,
		.len = len,
	};
}

bool
lexer_next(Lexer *lex, Token *tok)
{
	if (!lexer_skip(lex, tok))
	{
		return true;
	}

	lex->soff = lex->off;
	char c = lexer_eat(lex);

	if (c == '\0')
	{
		lexer_tok(lex, tok, TOK_EOF);
		return false;
	}
	else if (isalpha(c))
	{
		lexer_ident(lex, tok);
		return true;
	}

	Token_Type type;
	switch (c)
	{
		case '-':
			type = lexer_match(lex, '>') ? TOK_ARROW : TOK_MINUS;
			break;

		case '+':
			type = TOK_PLUS;
			break;

		case '*':
			type = TOK_STAR;
			break;

		case '/':
			type = TOK_SLASH;
			break;

		case '(':
			type = lexer_match(lex, ')') ? TOK_UNIT : TOK_LPAR;
			break;

		case ')':
			type = TOK_RPAR;
			break;

		case '.':
			type = TOK_DOT;
			break;

		case ',':
			type = TOK_COMMA;
			break;

		case '=':
			type = TOK_EQ;
			break;

		case '>':
			type = lexer_match(lex, '=') ? TOK_GTE : TOK_GT;
			break;

		case '<':
			type = lexer_match(lex, '=') ? TOK_LTE : TOK_LT;
			break;

		case '&':
			if (!lexer_match(lex, '&'))
			{
				lexer_err(lex, tok, "Unexpected character after `&`");
				return true;
			}
			type = TOK_AND;
			break;

		case '|':
			if (!lexer_match(lex, '|'))
			{
				lexer_err(lex, tok, "Unexpected character after `|`");
				return true;
			}
			type = TOK_OR;
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
			lexer_err(lex, tok, "Unexpected character");
			return true;
		}

	lexer_tok(lex, tok, type);
	return true;
}
