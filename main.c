#include <stdio.h>

#include "ast.h"

int main(int argc, char **argv)
{
    expr_t *expr = expr_apply_new(expr_lambda_new("x", expr_var_new("x")), expr_var_new("y"));
    expr_println(expr);
    expr_free(expr);
    return 0;
}
