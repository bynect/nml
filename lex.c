#include <ctype.h>
#include <string.h>

#include "lex.h"

void lex_init(lex_t *lex, const char *src, size_t len)
{
    lex->line = 1;
    lex->src = src;
    lex->len = len;
    lex->off = 0;
}

static bool lex_eof(lex_t *lex)
{
    return lex->off >= lex->len;
}

static void lex_token(lex_t *lex, token_t *tok, token_type_t type)
{
    tok->type = type;
    tok->str = lex->src + lex->tok_off;
    tok->len = lex->off - lex->tok_off;
    tok->line = lex->tok_line;
}

static void lex_error(lex_t *lex, token_t *tok, const char *error)
{
    tok->type = TOK_ERROR;
    tok->str = error;
    tok->len = strlen(error);
    tok->line = lex->tok_line;
}

static char lex_peek(lex_t *lex)
{
    return lex_eof(lex) ? '\0' : lex->src[lex->off];
}

static void lex_skip(lex_t *lex)
{
    while (true) {
        char c = lex_peek(lex);
        if (c == '\n')
            lex->line++;

        if (!isspace(c))
            return;

        lex->off++;
    }
}

static bool lex_ident(lex_t *lex, token_t *next)
{
    char c;
    do {
        lex->off++;
        c = lex_peek(lex);
    } while (isalnum(c) || c == '_');

    lex_token(lex, next, TOK_IDENT);

    if (!strncmp("let", next->str, 3))
        next->type = TOK_LET;
    else if (!strncmp("in", next->str, 2))
        next->type = TOK_IN;

    return true;
}

static bool lex_number(lex_t *lex, token_t *next)
{
    char c;
    do {
        lex->off++;
        c = lex_peek(lex);
    } while (isdigit(c));

    lex_token(lex, next, TOK_NUMBER);
    return true;
}

static bool lex_string(lex_t *lex, token_t *next)
{
    char c;
    do {
        lex->off++;
        c = lex_peek(lex);
    } while (!lex_eof(lex) && c != '"');

    if (lex_eof(lex)) {
        lex_error(lex, next, "Unterminated string");
        return true;
    }

    lex->off++;
    lex_token(lex, next, TOK_STRING);
    return true;
}

bool lex_next(lex_t *lex, token_t *next)
{
    lex_skip(lex);

    lex->tok_off = lex->off;
    lex->tok_line = lex->line;

    if (lex_eof(lex))
        return false;

    char c = lex_peek(lex);
    if (isalpha(c) || c == '_')
        return lex_ident(lex, next);

    if (isdigit(c))
        return lex_number(lex, next);

    lex->off++;

    token_type_t type;
    switch (c) {
        case '"':
            return lex_string(lex, next);

        case '(':
            type = TOK_LPAR;
            break;

        case ')':
            type = TOK_RPAR;
            break;

        case '[':
            type = TOK_LBRACK;
            break;

        case ']':
            type = TOK_RBRACK;
            break;

        case ':':
            type = TOK_COL;
            if (lex_peek(lex) == ':') {
                lex->off++;
                type = TOK_COLCOL;
            }
            break;

        case ';':
            type = TOK_SEMI;
            break;

        case '\\':
            type = TOK_BACK;
            break;

        case '=':
            type = TOK_EQ;
            if (lex_peek(lex) == '=') {
                lex->off++;
                type = TOK_EQEQ;
            }
            break;

        case '-':
            if (lex_peek(lex) == '>') {
                lex->off++;
                type = TOK_ARROW;
                break;
            }
            // fall through

        default:
            lex_error(lex, next, "Unknown character");
            return true;
    }

    lex_token(lex, next, type);
    return true;
}

const char *tokens[TOK_ERROR + 1] = {
    "TOK_IDENT",
    "TOK_NUMBER",
    "TOK_STRING",
    "TOK_LET",
    "TOK_IN",
    "TOK_EQ",
    "TOK_EQEQ",
    "TOK_ARROW",
    "TOK_SEMI",
    "TOK_BACK",
    "TOK_LPAR",
    "TOK_RPAR",
    "TOK_LBRACK",
    "TOK_RBRACK",
    "TOK_COL",
    "TOK_COLCOL",
    "TOK_ERROR",
};
