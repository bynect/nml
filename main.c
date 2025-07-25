#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "compile.h"
#include "decl.h"
#include "infer.h"
#include "parse.h"

int main(int argc, const char **argv)
{
    bool debug = false;
    if (argc == 3 && !strcmp(argv[1], "--debug")) {
        argc--;
        argv[1] = argv[2];
        debug = true;
    }

    if (argc != 2) {
        printf("Usage: %s [--debug] PATH\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
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

    decl_t **decls = NULL;
    size_t n_decls = 0;

    decl_t *decl;
    while (!parse_eof(&parse)) {
        if (!parse_decl(&parse, &decl)) {
            printf("Aborted parsing\n");
            exit(1);
        }

        if (debug) {
            printf("Parsed decl: ");
            decl_println(decl);
        }

        decls = realloc(decls, ++n_decls * sizeof(decl_t *));
        decls[n_decls - 1] = decl;
    }

    infer_t infer;
    infer_init(&infer, NULL);

    for (size_t i = 0; i < n_decls; i++) {
        decl_t *decl = decls[i];

        if (!infer_decl(&infer, decl)) {
            puts("Failed to infer types");
            return 1;
        }

        if (debug) {
            printf("Typed decl: ");
            decl_println(decl);
        }
    }

    FILE *out = fopen("out.S", "wb");
    compile_t comp;
    compile_init(&comp, out);

    for (size_t i = 0; i < n_decls; i++) {
        decl_t *decl = decls[i];

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

    for (size_t i = 0; i < n_decls; i++)
        decl_free(decls[i]);

    puts("Compiling out.S");

    if (system("gcc out.S -g -fpie") < 0)
        perror("system");

    munmap(mapped, size);
    close(fd);
    return 0;
}
