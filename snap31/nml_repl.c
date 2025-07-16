#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include "nml_dbg.h"
#include "nml_expr.h"
#include "nml_comp.h"
#include "nml_err.h"
#include "nml_hash.h"
#include "nml_lex.h"
#include "nml_mac.h"
#include "nml_mem.h"
#include "nml_mod.h"
#include "nml_parse.h"
#include "nml_dump.h"
#include "nml_set.h"
#include "nml_str.h"
#include "nml_tab.h"
#include "nml_type.h"
#include "nml_fold.h"
#include "nml_comp.h"
#include "nml_ir.h"
#include "nml_check.h"

static const Type_Var var_poly = {
	.var = STR_STATIC("'a")
};

static const Sized_Str var_name[] = {STR_STATIC("'a")};

static Type type_var = TYPE_NONE;

static const Type_Fun fun_int = {
	.par = TYPE_INT,
	.ret = TYPE_INT,
};

static const Type_Fun fun_float = {
	.par = TYPE_FLOAT,
	.ret = TYPE_FLOAT,
};

static const Type_Fun fun_bool = {
	.par = TYPE_BOOL,
	.ret = TYPE_BOOL,
};

static Type_Fun fun_cmp = {
	.par = TYPE_NONE,
	.ret = TYPE_BOOL,
};

static const Type_Fun fun_bint = {
	.par = TYPE_INT,
	.ret = TYPE_TAG(TAG_FUN, &fun_int),
};

static const Type_Fun fun_bfloat = {
	.par = TYPE_FLOAT,
	.ret = TYPE_TAG(TAG_FUN, &fun_float),
};

static const Type_Fun fun_bbool = {
	.par = TYPE_BOOL,
	.ret = TYPE_TAG(TAG_FUN, &fun_bool),
};

static Type_Fun fun_bcmp = {
	.par = TYPE_NONE,
	.ret = TYPE_TAG(TAG_FUN, &fun_cmp),
};

static Type_Fun fun_panic = {
	.par = TYPE_STR,
	.ret = TYPE_NONE,
};

static const Type type_iadd = TYPE_TAG(TAG_FUN, &fun_bint);
static const Type type_isub = TYPE_TAG(TAG_FUN, &fun_bint);
static const Type type_imul = TYPE_TAG(TAG_FUN, &fun_bint);
static const Type type_idiv = TYPE_TAG(TAG_FUN, &fun_bint);
static const Type type_irem = TYPE_TAG(TAG_FUN, &fun_bint);

static const Type type_fadd = TYPE_TAG(TAG_FUN, &fun_bfloat);
static const Type type_fsub = TYPE_TAG(TAG_FUN, &fun_bfloat);
static const Type type_fmul = TYPE_TAG(TAG_FUN, &fun_bfloat);
static const Type type_fdiv = TYPE_TAG(TAG_FUN, &fun_bfloat);
static const Type type_frem = TYPE_TAG(TAG_FUN, &fun_bfloat);

static const Type type_gt = TYPE_TAG(TAG_FUN, &fun_bcmp);
static const Type type_lt = TYPE_TAG(TAG_FUN, &fun_bcmp);
static const Type type_ge = TYPE_TAG(TAG_FUN, &fun_bcmp);
static const Type type_le = TYPE_TAG(TAG_FUN, &fun_bcmp);
static const Type type_eq = TYPE_TAG(TAG_FUN, &fun_bcmp);
static const Type type_ne = TYPE_TAG(TAG_FUN, &fun_bcmp);

static const Type type_and = TYPE_TAG(TAG_FUN, &fun_bbool);
static const Type type_or = TYPE_TAG(TAG_FUN, &fun_bbool);

static const Type type_ineg = TYPE_TAG(TAG_FUN, &fun_int);
static const Type type_fneg = TYPE_TAG(TAG_FUN, &fun_float);
static const Type type_not = TYPE_TAG(TAG_FUN, &fun_bool);

static const Type_Scheme scheme_eq = {
	.type = type_eq,
	.vars = (Sized_Str *)var_name,
	.len = 1,
};

static const Type_Scheme scheme_ne = {
	.type = type_ne,
	.vars = (Sized_Str *)var_name,
	.len = 1,
};

static const Type_Scheme scheme_panic = {
	.type = TYPE_TAG(TAG_FUN, &fun_panic),
	.vars = (Sized_Str *)var_name,
	.len = 1,
};

static MAC_INLINE inline void
repl_dump(const char *str)
{
	fprintf(stdout, "%s", str);
	fflush(stdout);
}

static void
repl_eval(Arena *arena, Scheme_Tab *ctx, Hash_Default seed, const Source src)
{
	Parser parse;
	parser_init(&parse, arena, &src);

	bool succ = false;
	Error_List err;
	error_init(&err);

	struct timespec start = {0, 0}, end = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &start);

	Ast_Top ast;
	if (parser_parse(&parse, &ast, &err))
	{

		scheme_tab_insert(ctx, STR_STATIC("+"), SCHEME_EMPTY(type_iadd));
		scheme_tab_insert(ctx, STR_STATIC("-"), SCHEME_EMPTY(type_isub));
		scheme_tab_insert(ctx, STR_STATIC("*"), SCHEME_EMPTY(type_imul));
		scheme_tab_insert(ctx, STR_STATIC("/"), SCHEME_EMPTY(type_idiv));
		scheme_tab_insert(ctx, STR_STATIC("%"), SCHEME_EMPTY(type_irem));

		scheme_tab_insert(ctx, STR_STATIC("+."), SCHEME_EMPTY(type_fadd));
		scheme_tab_insert(ctx, STR_STATIC("-."), SCHEME_EMPTY(type_fsub));
		scheme_tab_insert(ctx, STR_STATIC("*."), SCHEME_EMPTY(type_fmul));
		scheme_tab_insert(ctx, STR_STATIC("/."), SCHEME_EMPTY(type_fdiv));
		scheme_tab_insert(ctx, STR_STATIC("%."), SCHEME_EMPTY(type_frem));

		scheme_tab_insert(ctx, STR_STATIC(">"), SCHEME_EMPTY(type_gt));
		scheme_tab_insert(ctx, STR_STATIC("<"), SCHEME_EMPTY(type_lt));
		scheme_tab_insert(ctx, STR_STATIC(">="), SCHEME_EMPTY(type_ge));
		scheme_tab_insert(ctx, STR_STATIC("<="), SCHEME_EMPTY(type_le));
		scheme_tab_insert(ctx, STR_STATIC("="), scheme_eq);
		scheme_tab_insert(ctx, STR_STATIC("!="), scheme_ne);

		scheme_tab_insert(ctx, STR_STATIC("&&"), SCHEME_EMPTY(type_and));
		scheme_tab_insert(ctx, STR_STATIC("||"), SCHEME_EMPTY(type_or));

		scheme_tab_insert(ctx, STR_STATIC("neg"), SCHEME_EMPTY(type_ineg));
		scheme_tab_insert(ctx, STR_STATIC("fneg"), SCHEME_EMPTY(type_fneg));
		scheme_tab_insert(ctx, STR_STATIC("not"), SCHEME_EMPTY(type_not));

		scheme_tab_insert(ctx, STR_STATIC("panic"), scheme_panic);

		Type_Checker check;
		checker_init(&check, seed, arena, &src, ctx);
		succ = checker_infer(&check, &ast, &err);
	}

	Module mod;
	module_init(&mod, src.name);

	if (succ)
	{
		//Folder fold;
		//folder_init(&fold, arena, seed);
		//folder_ast(&fold, &ast, &err);

		//Compiler comp;
		//compiler_init(&comp, seed, arena, &mod);
		//succ = compiler_compile(&comp, &ast, &err);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	fprintf(stdout, "Pipeline took %lf ms\n",
			(end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1e6);

	dump_error(&err);
	fprintf(stdout, "Lex dump:\n");

	Lexer lex;
	lexer_init(&lex, &src);
	dump_lexer(&lex);

	if (succ)
	{
		fprintf(stdout, "Ast dump:\n");
		dump_ast(&ast);

		fprintf(stdout, "Ir dump:\n");
		dump_module(&mod);
	}
	scheme_tab_reset(ctx);

	fprintf(stdout, "Arena dump:\n");
	dump_arena(arena);

	arena_reset(arena);
	arena_shrink(arena, ARENA_PAGE * 4);
}

static inline void
repl_eof(Arena *arena, Scheme_Tab *ctx, Hash_Default seed)
{
	char tmp[BUFSIZ];
	size_t off = 0;

	size_t size = BUFSIZ * 2;
	char *src = arena_lock(arena, size);

	while (fgets(tmp, BUFSIZ, stdin) != NULL)
	{
		size_t len = strlen(tmp);
		if (off + len >= size)
		{
			size *= 2;
			src = arena_update(arena, src, size);
		}

		memcpy(src + off, tmp, len);
		off += len;
	}

	src = arena_update(arena, src, off);
	arena_unlock(arena, src);
	repl_eval(arena, ctx, seed, (Source) {STR_STATIC("@repl"), STR_WLEN(src, off)});
}

static inline void
repl_run(Arena *arena, Scheme_Tab *ctx, Hash_Default seed)
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
		repl_eval(arena, ctx, seed, (Source) {STR_STATIC("@repl"), STR_ZLEN(src)});
		repl_dump("# ");
	}
	putc('\n', stdout);
}

static inline Sized_Str
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
	char *src = arena_alloc(arena, len);

	rewind(file);
	if (fread(src, 1, len, file) < len)
	{
		fprintf(stderr, "Error reading file\n");
		fclose(file);
		exit(1);
	}

	fclose(file);
	return STR_WLEN(src, len);
}


static inline Hash_Default
repl_seed(void)
{
	FILE *file = fopen("/dev/urandom", "rb");
	Hash_Default seed;

	if (file == NULL)
	{
		seed = (long)&file;
		return seed;
	}

 	if (fread(&seed, sizeof(Hash_Default), 1, file) != sizeof(Hash_Default))
	{
		fclose(file);
		seed = (long)&file;
		return seed;
	}

	fclose(file);
	return seed;
}

int
main(int argc, const char **argv)
{
	type_var = TYPE_TAG(TAG_VAR, &var_poly);
	fun_cmp.par = type_var;
	fun_bcmp.par = type_var;
	fun_panic.ret = type_var;

	Hash_Default seed = repl_seed();

	Arena arena;
	arena_init(&arena, ARENA_PAGE * 4);

	Scheme_Tab ctx;
	scheme_tab_init(&ctx, seed);

	if (argc == 2 && !strcmp(argv[1], "--eof"))
	{
		repl_eof(&arena, &ctx, seed);
	}
	else if (argc == 3 && !strcmp(argv[1], "--file"))
	{
		Sized_Str src = repl_file(&arena, argv[2]);
		repl_eval(&arena, &ctx, seed, (Source) {STR_ZLEN(argv[2]), src});
	}
	else if (argc == 1)
	{
		repl_run(&arena, &ctx, seed);
	}
	else
	{
		fprintf(stderr, "Unexpected argument\n");
		scheme_tab_free(&ctx);
		arena_free(&arena);
		return 1;
	}

	scheme_tab_free(&ctx);
	arena_free(&arena);
	return 0;
}
