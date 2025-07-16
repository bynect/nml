//#include <stddef.h>
//#include <stdio.h>
//#include <string.h>
//#include <time.h>
//
//// FIXME: sigaction() is not portable
//#include <signal.h>
//
//#include "nml_arena.h"
//#include "nml_mac.h"
//#include "nml_str.h"
//#include "nml_hash.h"
//#include "nml_loc.h"
//
//static inline void
//main_seed(Hash_Default *seed)
//{
//	FILE *file = fopen("/dev/urandom", "rb");
//	file = file != NULL ? file : fopen("/dev/random", "rb");
//
//	double time = time(NULL);
//	if (file == NULL)
//	{
//		*seed = (Hash_Default)&file + ((Hash_Default)time << 2);
//	}
//	else
//	{
//		setbuf(file, NULL);
//
//		if (fread(seed, sizeof(Hash_Default), 1, file) != sizeof(Hash_Default))
//		{
//			*seed = (Hash_Default)&file + ((Hash_Default)time << 2);
//		}
//
//		fclose(file);
//	}
//}
//
//int
//main(int argc, const char **argv)
//{
//	if (argc > 2)
//	{
//		fprintf(stderr, "Usage: nml [file]\n");
//		return 1;
//	}
//
//	Arena arena;
//	arena_init(&arena, ARENA_PAGE);
//
//	Hash_Default seed;
//	main_seed(&seed);
//
//	if (argc == 1)
//	{
//		struct sigaction act;
//		memset(&act, 0, sizeof(struct sigaction));
//		act.sa_handler = SIG_IGN;
//		act.sa_flags = SA_RESTART;
//		sigaction(SIGINT, &act, NULL);
//
//		char src[BUFSIZ];
//		fprintf(stdout, "NML repl [%s %s]\n#", MAC_DATE, MAC_TIME);
//		fflush(stdout);
//
//		while (fgets(src, BUFSIZ, stdin) != NULL)
//		{
//			main_(&arena, seed, (Source) {STR_STATIC("@repl"), STR_ZLEN(src)});
//
//			fprintf(stdout, "# ");
//			fflush(stdout);
//		}
//
//		putc('\n', stdout);
//	}
//	else if (argc == 2)
//	{
//		FILE *file = !strcmp(argv[1], "-") ? stdin : fopen(argv[1], "rb");
//
//		if (file == NULL)
//		{
//			fprintf(stderr, "Error opening file\n");
//			return 1;
//		}
//
//
//
//		fclose(file);
//	}
//
//	arena_free(&arena);
//	return 0;
//}
