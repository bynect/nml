#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "nml_comp.h"
#include "nml_dbg.h"
#include "nml_decl.h"
#include "nml_expr.h"
#include "nml_mac.h"
#include "nml_str.h"
#include "nml_tab.h"

static MAC_INLINE inline void
compiler_error(Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
}

static MAC_INLINE inline Var_Id
compiler_id(Compiler *comp)
{
	return comp->id++;
}

static MAC_HOT void
compiler_expr(Compiler *comp, Expr_Node *expr)
{
}

static MAC_HOT void
compiler_decl_let(Compiler *comp, Decl_Let *node)
{
}

static MAC_HOT void
compiler_decl_fun(Compiler *comp, Decl_Fun *node)
{
}

static MAC_HOT void
compiler_decl(Compiler *comp, Decl_Node *node)
{
	switch (node->kind)
	{
		case DECL_LET:
			compiler_decl_let(comp, (Decl_Let *)node);
			break;

		case DECL_FUN:
			compiler_decl_fun(comp, (Decl_Fun *)node);
			break;

		case DECL_DATA:
			// TODO
			break;

		case DECL_TYPE:
			// TODO
			break;
	}
}

void
compiler_init(Compiler *comp, Arena *arena, Hash_Default seed)
{
	comp->arena = arena;
	comp->err = NULL;
	comp->succ = true;

	comp->id = 0;
	id_tab_init(&comp->tab, seed);
}

void
compiler_free(Compiler *comp)
{
	id_tab_free(&comp->tab);
}

MAC_HOT bool
compiler_compile(Compiler *comp, Ast_Top *ast, Error_List *err)
{
	comp->err = err;
	bool succ = true;

	for (size_t i = 0; i < ast->len; ++i)
	{
		compiler_decl(comp, ast->decls[i]);

		if (!comp->succ)
		{
			succ = false;
			comp->succ = true;
		}
	}

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
		if (ast->decls[i]->kind == DECL_LET)
		{
			Decl_Let *let = (Decl_Let *)ast->decls[i];
			if (STR_EQUAL(let->name, STR_STATIC("_")))
			{
				continue;
			}
		}
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

			uint32_t id;
			if (id_tab_get(&comp->tab, let->name, &id))
			{
				fprintf(stdout, "v_%d = ", id);
				compiler_expr(comp, let->value);
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
