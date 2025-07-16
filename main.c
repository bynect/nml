#include <stdio.h>
#include <stdlib.h>

#include "compile.h"
#include "expr.h"
#include "infer.h"

int main(int argc, char **argv)
{
    //expr_t *expr = expr_apply_new(expr_lambda_new("x", expr_var_new("x")), expr_lit_new(42));

    //expr_t *expr = expr_lit_new(42);

    //expr_t *expr = expr_apply_new(
    //    expr_apply_new(
    //        expr_apply_new(
    //            expr_lambda_new(
    //                "x",
    //                expr_let_new(
    //                    "y",
    //                    expr_lambda_new(
    //                        "z",
    //                        expr_lambda_new(
    //                            "k",
    //                            expr_var_new("x")
    //                        )
    //                    ),
    //                    expr_var_new("y")
    //                )
    //            ),
    //            expr_lit_new(69)
    //        ),
    //        expr_lit_new(42)
    //    ),
    //    expr_let_new(
    //        "p",
    //        expr_lit_new(104),
    //        expr_var_new("p")
    //    )
    //);

    expr_t *expr = expr_let_new(
        "id",
        expr_lambda_new(
            "x",
            expr_var_new("x")
        ),
        expr_apply_new(
            expr_var_new("id"),
            expr_lit_new(42)
        )
    );

    printf("Expr: ");
    expr_println(expr);

    infer_t infer;
    infer_init(&infer, NULL);

    if (!infer_expr(&infer, expr)) {
        puts("Failed to infer types");
        expr_free(expr);
        return 1;
    }

    infer_resolve(&infer, expr);

    printf("Typed: ");
    expr_println(expr);

    FILE *out = fopen("out.S", "wb");
    compile_t comp;
    compile_init(&comp, out);

    bool ok = compile_expr(&comp, expr);
    fclose(out);
    expr_free(expr);

    if (ok) {
        puts("Compiling out.S");
        if (system("gcc out.S -g") < 0)
            perror("system");
    } else {
        puts("Failed to compile");
    }

    return 0;
}
