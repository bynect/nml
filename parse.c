#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "decl.h"
#include "lex.h"

static void parse_next(parse_t *parse)
{
    lex_next(&parse->lex, &parse->next);
}

void parse_init(parse_t *parse, const char *src, size_t len)
{
    lex_init(&parse->lex, src, len);
    parse_next(parse);
}

bool parse_eof(parse_t *parse)
{
    return parse->next.type == TOK_EOF;
}

static bool parse_check(parse_t *parse, token_type_t type)
{
    return parse->next.type == type;
}

static bool parse_match(parse_t *parse, token_type_t type)
{
    if (parse_check(parse, type)) {
        parse_next(parse);
        return true;
    }
    return false;
}

static bool parse_expect(parse_t *parse, token_type_t type)
{
    if (parse_match(parse, type))
        return true;

    printf("%u: Expected token `%s` but got `%s`\n",
           parse->next.line,
           tokens[type],
           tokens[parse->next.type]);
    return false;
}

bool parse_expr(parse_t *parse, expr_t **expr)
{
    *expr = expr_lit_new_int(1);
    return true;
}

bool parse_decl_let(parse_t *parse, decl_t **decl)
{
    token_t var = parse->next;
    if (!parse_expect(parse, TOK_IDENT))
        return false;

    if (!parse_expect(parse, TOK_EQ))
        return false;

    expr_t *expr;
    if (!parse_expr(parse, &expr))
        return false;

    char *bound = strndup(var.str, var.len);
    *decl = decl_let_new(bound, expr);
    return true;
}

bool parse_decl(parse_t *parse, decl_t **decl)
{
    if (parse_match(parse, TOK_LET))
        return parse_decl_let(parse, decl);

    printf("%u: Unexpected token type: %s\n",
           parse->next.line, tokens[parse->next.type]);

    return false;
}
