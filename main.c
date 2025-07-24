#include <stdio.h>
#include <stdlib.h>

#include "compile.h"
#include "decl.h"
#include "expr.h"
#include "infer.h"

int main(int argc, char **argv)
{
    expr_t *expr = expr_apply_new(
        expr_apply_new(
            expr_apply_new(
                expr_lambda_new(
                    "x",
                    expr_let_new(
                        "y",
                        expr_lambda_new(
                            "z",
                            expr_lambda_new(
                                "k",
                                expr_var_new("x")
                            )
                        ),
                        expr_var_new("y")
                    )
                ),
                expr_lit_new_int(69)
            ),
            expr_lit_new_int(42)
        ),
        expr_let_new(
            "p",
            expr_lit_new_str("hello"),
            expr_var_new("p")
        )
    );

    decl_t *decls[] = {
        decl_let_new("id", expr_lambda_new("x", expr_var_new("x"))),
        decl_let_new("sus", expr_apply_new(expr_var_new("id"), expr_lit_new_int(10))),
        decl_let_new("main", expr),
        NULL
    };

    infer_t infer;
    infer_init(&infer, NULL);

    for (decl_t **ptr = decls; *ptr; ptr++) {
        decl_t *decl = *ptr;

        printf("Decl: ");
        decl_println(decl);

        if (!infer_decl(&infer, decl)) {
            puts("Failed to infer types");
            expr_free(expr);
            return 1;
        }

        printf("Typed: ");
        decl_println(decl);
    }

    FILE *out = fopen("out.S", "wb");
    compile_t comp;
    compile_init(&comp, out);

    for (decl_t **ptr = decls; *ptr; ptr++) {
        decl_t *decl = *ptr;

        if (!compile_decl(&comp, decl)) {
            fputs("Failed to compile ", stdout);
            decl_println(decl);
            return 1;
        }
    }

    // TODO: Fix errors
    if (!compile_main(&comp)) {
        printf("Failed to emit main function\n");
        return 1;
    }

    compile_free(&comp);

    fclose(out);
    puts("Compiling out.S");

    if (system("gcc out.S -g") < 0)
        perror("system");

    return 0;
}
