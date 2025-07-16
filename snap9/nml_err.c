#include "nml_err.h"
#include "nml_dbg.h"

void
error_init(Error_List *err)
{
	err->start = NULL;
	err->last = NULL;
}

MAC_HOT void
error_append(Error_List *err, Arena *arena, Sized_Str msg, Location loc, Error_Flag flags)
{
	Error *ptr;
	if (err->start == NULL)
	{
		err->start = arena_alloc(arena, sizeof(Error));
		err->last = err->start;
		ptr = err->start;
	}
	else
	{
		ptr = arena_alloc(arena, sizeof(Error));
		err->last->next = ptr;
		err->last = ptr;
	}

	ptr->msg = msg;
	ptr->loc = loc;
	ptr->flags = flags;
	ptr->next = NULL;
}

void
error_iter(Error_List *err, Error_Iter_Fn fn, void *ctx)
{
	Error *ptr = err->start;
	while (ptr != NULL)
	{
		fn(ptr, ctx);
		ptr = ptr->next;
	}
}
