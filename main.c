#include <stdio.h>

#include "ast.h"
#include "compile.h"

int main(int argc, char **argv)
{
    //expr_t *expr = expr_apply_new(expr_lambda_new("x", expr_var_new("x")), expr_lit_new(42));

    expr_t *expr = expr_lit_new(42);

    expr_println(expr);

    FILE *out = fopen("out.S", "wb");

    compile_t comp;
    compile_init(&comp, out);

    if (!compile_expr(&comp, expr)) {
        puts("Failed to compile");
    }

    fclose(out);

    expr_free(expr);
    return 0;
}
