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
compiler_error(Compiler *comp, Location loc, Sized_Str msg)
{
	error_append(comp->err, comp->arena, msg, loc, ERR_ERROR);
	comp->succ = false;
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

	byte_buf_init(&comp->buf);
}

void
compiler_free(Compiler *comp)
{
	byte_buf_free(&comp->buf);
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

	return succ;
}
