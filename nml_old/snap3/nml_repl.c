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

static Type_Fun fun_int = {
	.par = TYPE_INT,
	.ret = TYPE_INT,
};

static Type_Fun fun_float = {
	.par = TYPE_FLOAT,
	.ret = TYPE_FLOAT,
};

static Type_Fun fun_bool = {
	.par = TYPE_BOOL,
	.ret = TYPE_BOOL,
};

static Type_Fun fun_cmp = {
	.par = TYPE_INT,
	.ret = TYPE_BOOL,
};

static Type_Fun fun_bint = {
	.par = TYPE_INT,
	.ret = TYPE_TAG(TAG_FUN, &fun_int),
};

static Type_Fun fun_bfloat = {
	.par = TYPE_FLOAT,
	.ret = TYPE_TAG(TAG_FUN, &fun_float),
};

static Type_Fun fun_bbool = {
	.par = TYPE_BOOL,
	.ret = TYPE_TAG(TAG_FUN, &fun_bool),
};

static Type_Fun fun_bcmp = {
	.par = TYPE_INT,
	.ret = TYPE_TAG(TAG_FUN, &fun_cmp),
};

static Type type_iadd = TYPE_TAG(TAG_FUN, &fun_bint);
static Type type_isub = TYPE_TAG(TAG_FUN, &fun_bint);
static Type type_imul = TYPE_TAG(TAG_FUN, &fun_bint);
static Type type_idiv = TYPE_TAG(TAG_FUN, &fun_bint);
static Type type_irem = TYPE_TAG(TAG_FUN, &fun_bint);

static Type type_fadd = TYPE_TAG(TAG_FUN, &fun_bfloat);
static Type type_fsub = TYPE_TAG(TAG_FUN, &fun_bfloat);
static Type type_fmul = TYPE_TAG(TAG_FUN, &fun_bfloat);
static Type type_fdiv = TYPE_TAG(TAG_FUN, &fun_bfloat);

static Type type_gt = TYPE_TAG(TAG_FUN, &fun_bcmp);
static Type type_lt = TYPE_TAG(TAG_FUN, &fun_bcmp);
static Type type_ge = TYPE_TAG(TAG_FUN, &fun_bcmp);
static Type type_le = TYPE_TAG(TAG_FUN, &fun_bcmp);
static Type type_eq = TYPE_TAG(TAG_FUN, &fun_bcmp);
static Type type_ne = TYPE_TAG(TAG_FUN, &fun_bcmp);

static Type type_and = TYPE_TAG(TAG_FUN, &fun_bbool);
static Type type_or = TYPE_TAG(TAG_FUN, &fun_bbool);

static Type type_ineg = TYPE_TAG(TAG_FUN, &fun_int);
static Type type_fneg = TYPE_TAG(TAG_FUN, &fun_float);
static Type type_not = TYPE_TAG(TAG_FUN, &fun_bool);

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
		clock_gettime(CLOCK_MONOTONIC, &start);

		Type_Tab ctx;
		type_tab_init(&ctx, 0);

		type_tab_set(&ctx, arena, STR_STATIC("+"), type_iadd);
		type_tab_set(&ctx, arena, STR_STATIC("-"), type_isub);
		type_tab_set(&ctx, arena, STR_STATIC("*"), type_imul);
		type_tab_set(&ctx, arena, STR_STATIC("/"), type_idiv);
		type_tab_set(&ctx, arena, STR_STATIC("%"), type_irem);

		type_tab_set(&ctx, arena, STR_STATIC("+."), type_fadd);
		type_tab_set(&ctx, arena, STR_STATIC("-."), type_fsub);
		type_tab_set(&ctx, arena, STR_STATIC("*."), type_fmul);
		type_tab_set(&ctx, arena, STR_STATIC("/."), type_fdiv);

		type_tab_set(&ctx, arena, STR_STATIC(">"), type_gt);
		type_tab_set(&ctx, arena, STR_STATIC("<"), type_lt);
		type_tab_set(&ctx, arena, STR_STATIC(">="), type_ge);
		type_tab_set(&ctx, arena, STR_STATIC("<="), type_le);
		type_tab_set(&ctx, arena, STR_STATIC("="), type_eq);
		type_tab_set(&ctx, arena, STR_STATIC("!="), type_ne);

		type_tab_set(&ctx, arena, STR_STATIC("&&"), type_and);
		type_tab_set(&ctx, arena, STR_STATIC("||"), type_or);

		type_tab_set(&ctx, arena, STR_STATIC("~-"), type_ineg);
		type_tab_set(&ctx, arena, STR_STATIC("~-."), type_fneg);
		type_tab_set(&ctx, arena, STR_STATIC("~!"), type_not);

		Type_Checker check;
		checker_init(&check, arena, 0, &ctx);
		succ = checker_infer_ast(&check, &ast, &err);
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
