#include "nml_fun.h"
#include "nml_mac.h"

void
function_init(Function *fun, size_t len)
{
	fun->len = len;
	fun->head = NULL;
	fun->tail = NULL;
}

MAC_HOT void
function_append(Function *fun, Block *block)
{
	if (MAC_UNLIKELY(fun->head == NULL))
	{
		fun->head = block;
		fun->tail = fun->head;
	}
	else
	{
		fun->tail->next = block;
		fun->tail = block;
	}
}
