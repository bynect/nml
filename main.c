#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "compile.h"
#include "decl.h"
#include "expr.h"
#include "infer.h"
#include "parse.h"

int test_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t size = st.st_size;
    void *mapped = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    parse_t parse;
    parse_init(&parse, mapped, size);

    decl_t *decl;
    while (!parse_eof(&parse)) {
        if (!parse_decl(&parse, &decl)) {
            break;
        }

        puts("Parsed decl:");
        decl_println(decl);
        decl_free(decl);
    }

    munmap(mapped, size);
    return 0;
}

int main(int argc, const char **argv)
{
    if (argc == 2) {
        return test_file(argv[1]);
    }

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
                expr_lit_new_str("helloworld")
            ),
            expr_lit_new_int(42)
        ),
        expr_let_new(
            "p",
            expr_lit_new_str("hello"),
            expr_var_new("p")
        )
    );

    expr_t *print = expr_apply_new(
        expr_apply_new(
            expr_var_new("ffi_call"),
            expr_var_new("puts")
        ),
        expr
    );

    decl_t *decls[] = {
        decl_let_new("id", expr_lambda_new("x", expr_var_new("x"))),
        decl_let_new("puts", expr_apply_new(expr_var_new("ffi_extern"), expr_lit_new_str("puts"))),
        decl_let_new("main", print),
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

    if (system("gcc out.S -g -fpie") < 0)
        perror("system");

    return 0;
}
