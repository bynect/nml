#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "decl.h"
#include "expr.h"
#include "lex.h"

static void parse_next(parse_t *parse)
{
    while (true) {
        lex_next(&parse->lex, &parse->next);
        if (parse->next.type != TOK_ERROR)
            break;

        printf("%u: %.*s\n",
               parse->next.line,
               (int)parse->next.len,
               parse->next.str);
    }
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

static bool parse_check_delim(parse_t *parse)
{
    return !parse_check(parse, TOK_LPAR)
        && !parse_check(parse, TOK_SEMI)
        && !parse_check(parse, TOK_IN)
        && !parse_eof(parse);
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
           parse->next.line, tokens[type], tokens[parse->next.type]);
    return false;
}

bool parse_expr(parse_t *parse, expr_t **expr);

static bool parse_expr_lambda(parse_t *parse, expr_t **expr)
{
    token_t var = parse->next;
    if (!parse_expect(parse, TOK_IDENT))
        return false;

    if (!parse_expect(parse, TOK_ARROW))
        return false;

    expr_t *body;
    if (!parse_expr(parse, &body))
        return false;

    char *bound = strndup(var.str, var.len);
    *expr = expr_lambda_new(bound, body);
    return true;
}

static bool parse_expr_let(parse_t *parse, expr_t **expr)
{
    token_t var = parse->next;
    if (!parse_expect(parse, TOK_IDENT))
        return false;

    if (!parse_expect(parse, TOK_EQ))
        return false;

    expr_t *value;
    if (!parse_expr(parse, &value))
        return false;

    if (!parse_expect(parse, TOK_IN))
        return false;

    expr_t *body;
    if (!parse_expr(parse, &body))
        return false;

    char *bound = strndup(var.str, var.len);
    *expr = expr_let_new(bound, value, body);
    return true;
}

static bool parse_expr_simple(parse_t *parse, expr_t **expr)
{
    switch (parse->next.type) {
        case TOK_IDENT: {
            char *var = strndup(parse->next.str, parse->next.len);
            *expr = expr_var_new(var);
            parse_next(parse);
            break;
        }

        case TOK_NUMBER: {
            int64_t value = strtoll(parse->next.str, NULL, 10);
            *expr = expr_lit_new_int(value);
            parse_next(parse);
            break;
        }

        case TOK_STRING: {
            char *str = strndup(parse->next.str + 1, parse->next.len - 2);
            *expr = expr_lit_new_str(str);
            parse_next(parse);
            break;
        }

        case TOK_BACK:
            parse_next(parse);
            return parse_expr_lambda(parse, expr);

        case TOK_LPAR:
            parse_next(parse);
            return parse_expr(parse, expr)
                && parse_expect(parse, TOK_RPAR);

        default:
            printf("%u: Unexpected token `%s` in expression\n",
                   parse->next.line, tokens[parse->next.type]);
            return false;

    }
    return true;
}

bool parse_expr(parse_t *parse, expr_t **expr)
{
    if (parse_match(parse, TOK_BACK))
        return parse_expr_lambda(parse, expr);

    if (parse_match(parse, TOK_LET))
        return parse_expr_let(parse, expr);

    if (!parse_expr_simple(parse, expr))
        return false;

    while (parse_check_delim(parse)) {
        expr_t *arg;
        if (!parse_expr_simple(parse, &arg))
            return false;

        *expr = expr_apply_new(*expr, arg);
    }
    return true;
}

bool parse_decl_let(parse_t *parse, decl_t **decl)
{
    token_t var = parse->next;
    if (!parse_expect(parse, TOK_IDENT))
        return false;

    if (!parse_expect(parse, TOK_EQ))
        return false;

    expr_t *value;
    if (!parse_expr(parse, &value))
        return false;

    if (!parse_expect(parse, TOK_SEMI))
        return false;

    char *bound = strndup(var.str, var.len);
    *decl = decl_let_new(bound, value);
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
