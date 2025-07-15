#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "compile.h"

int main(int argc, char **argv)
{
    //expr_t *expr = expr_apply_new(expr_lambda_new("x", expr_var_new("x")), expr_lit_new(42));

    //expr_t *expr = expr_lit_new(42);

    // ((\x -> \y -> x) 69) 42
    expr_t *expr = expr_apply_new(
        expr_apply_new(
            expr_lambda_new(
                "x",
                expr_lambda_new(
                    "y",
                    expr_var_new("x")
                )
            ),
            expr_lit_new(69)
        ),
        expr_lit_new(42)
    );

    expr_println(expr);

    FILE *out = fopen("out.S", "wb");

    compile_t comp;
    compile_init(&comp, out);

    bool ok = compile_expr(&comp, expr);
    fclose(out);

    expr_free(expr);

    if (ok) {
        puts("Compiling out.S");
        if (system("gcc out.S") < 0)
            perror("system");
    } else {
        puts("Failed to compile");
    }

    return 0;
}
