#include "nml_err.h"
#include "nml_dbg.h"
#include "nml_mac.h"

void
error_init(Error_List *err)
{
	err->head = NULL;
	err->tail = NULL;
}

MAC_HOT void
error_append(Error_List *err, Arena *arena, Sized_Str msg,
			Location loc, Error_Flag flags)
{
	Error *ptr;
	if (MAC_UNLIKELY(err->head == NULL))
	{
		err->head = arena_alloc(arena, sizeof(Error));
		err->tail = err->head;
		ptr = err->head;
	}
	else
	{
		ptr = arena_alloc(arena, sizeof(Error));
		err->tail->next = ptr;
		err->tail = ptr;
	}

	ptr->msg = msg;
	ptr->loc = loc;
	ptr->flags = flags;
	ptr->next = NULL;
}

void
error_iter(Error_List *err, Error_Iter_Fn fn, void *ctx)
{
	Error *ptr = err->head;
	while (ptr != NULL)
	{
		fn(ptr, ctx);
		ptr = ptr->next;
	}
}
