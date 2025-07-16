#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "nml_comp.h"
#include "nml_dbg.h"
#include "nml_decl.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"

static MAC_INLINE inline void
compiler_error(C_Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
}

static MAC_INLINE inline void
compiler_write(Location loc, const char *fmt, ...)
{
	fprintf(stdout, "#file %d \"%.*s\"\n", loc.line,
			(uint32_t)loc.src->name.len, loc.src->name.str);

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

static MAC_HOT void
compiler_type()
{
}

static MAC_HOT void
compiler_decl(C_Compiler *comp, Decl_Node *node)
{

}

void
compiler_init(C_Compiler *comp, Arena *arena, Hash_Default seed)
{
	comp->arena = arena;
	comp->err = NULL;
	comp->succ = true;

	str_tab_init(&comp->tab, seed);
}

void
compiler_free(C_Compiler *comp)
{
	str_tab_free(&comp->tab);
}

MAC_HOT bool
compiler_compile(C_Compiler *comp, Ast_Top *ast, Error_List *err)
{
	comp->err = err;
	bool succ = true;

	fprintf(stdout,
			"#include <stdio.h>\n"
			"#include <stdlib.h>\n"
			"#include <stdint.h>\n"
			"\n"
			"static inline void\n"
			"_nml_panic(const Sized_Str msg)\n"
			"{\n"
			"	fprintf(stderr, \"%%.*s\", (uint32_t)msg.len, msg.str);\n"
			"	abort();\n"
			"}\n\n");

	for (size_t i = 0; i < ast->len; ++i)
	{
		compiler_decl(comp, ast->decls[i]);
	}

	fprintf(stdout,
			"\nstatic inline void\n"
			"_nml_main()\n"
			"{\n");

	for (size_t i = 0; i < ast->len; ++i)
	{
		if (ast->decls[i]->kind == DECL_LET)
		{
			Decl_Let *let = (Decl_Let *)ast->decls[i];
			if (STR_EQUAL(let->name, STR_STATIC("_")))
			{
				compiler_write(let->node.loc,
								"");
			}
		}
	}

	fprintf(stdout, "}\n\n"
			"int\n"
			"main()\n"
			"{\n"
			"	_nml_main();\n"
			"	return 0;\n"
			"}\n");

	return succ;
}
