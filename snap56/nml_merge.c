#include "nml_merge.h"

void
merger_init(Merger *merge, Arena *arena)
{
	merge->arena = arena;
	merge->lambda_chain = false;
	merge->app_chain = false;
}

MAC_HOT void
merger_pass(Merger *merge, Ast_Top *ast)
{
}
