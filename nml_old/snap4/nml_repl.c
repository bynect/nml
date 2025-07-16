#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "nml_ast.h"
#include "nml_comp.h"
#include "nml_err.h"
#include "nml_lex.h"
#include "nml_mem.h"
#include "nml_parse.h"
#include "nml_dbg.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_type.h"
#include "nml_fold.h"
#include "nml_type.h"

static void
repl_dump(const char *str)
{
	fprintf(stdout, "%s", str);
	fflush(stdout);
}

static void
repl_eval(Arena *arena, const char *src, size_t len)
{
	Parser parse;
	parser_init(&parse, arena, STR_WLEN(src, len));

	bool succ = false;
	Error_List err;
	error_init(&err);

	struct timespec start = {0, 0}, end = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &start);

	Ast_Top ast;
	if (parser_parse(&parse, &ast, &err))
	{
		Scheme_Tab ctx;
		scheme_tab_init(&ctx, 0);

		Type_Checker check;
		checker_init(&check, arena, 0, &ctx);
		succ = checker_infer(&check, &ast, &err);
	}

	if (succ)
	{
		fold_ast(arena, &ast, 0);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stdout, "Pipeline took %lf ms\n",
			(end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1e6);

	dbg_dump_error(&err);
	if (succ)
	{
		fprintf(stdout, "Ast dump:\n");
		dbg_dump_ast(&ast);
	}

	fprintf(stdout, "Arena dump:\n");
	dbg_dump_arena(arena);

	arena_reset(arena, NULL);
	arena_shrink(arena, ARENA_PAGE * 4);
}

static void
repl_eof(Arena *arena)
{
	char src[BUFSIZ * 4];
	char tmp[BUFSIZ];

	size_t off = 0;
	size_t len = 0;

	while (fgets(tmp, BUFSIZ, stdin) != NULL)
	{
		len = strlen(tmp);
		memcpy(src + off, tmp, len);
		off += len;
	}
	repl_eval(arena, src, off);
}

static void
repl_file(Arena *arena, const char *name)
{
	FILE *file = fopen(name, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "Error opening file\n");
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	size_t len = ftell(file);
	char src[len];

	rewind(file);
	if (fread(src, 1, len, file) < len)
	{
		fprintf(stderr, "Error reading file\n");
		fclose(file);
		exit(1);
	}

	fclose(file);
	repl_eval(arena, src, len);
}

static void
repl_run(Arena *arena)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = SIG_IGN;
	act.sa_flags = SA_RESTART;
	sigaction(SIGINT, &act, NULL);

	char src[BUFSIZ];
	repl_dump("nml repl\n# ");

	while (fgets(src, BUFSIZ, stdin) != NULL)
	{
		repl_eval(arena, src, strlen(src));
		repl_dump("# ");
	}
	putc('\n', stdout);
}

int
main(int argc, const char **argv)
{
	Arena arena;
	arena_init(&arena, ARENA_PAGE * 4);

	if (argc == 2 && !strcmp(argv[1], "--eof"))
	{
		repl_eof(&arena);
	}
	else if (argc == 3 && !strcmp(argv[1], "--file"))
	{
		repl_file(&arena, argv[2]);
	}
	else if (argc == 1)
	{
		repl_run(&arena);
	}
	else
	{
		fprintf(stderr, "Unexpected argument\n");
		return 1;
	}

	arena_free(&arena);
	return 0;
}
