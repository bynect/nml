#include "nml_fun.h"
#include "nml_mac.h"

void
block_init(Block *block, Block_Id id, Instr *entry)
{
	block->id = id;
	block->head = entry;
	block->tail = block->head;
	block->next = NULL;
}

MAC_HOT void
block_append(Block *block, Instr *instr)
{
	if (MAC_UNLIKELY(block->head == NULL))
	{
		block->head = instr;
		block->tail = block->head;
	}
	else
	{
		block->tail->next = instr;
		block->tail = instr;
	}
}

void
function_init(Function *fun, size_t len)
{
	fun->len = len;
	fun->head = NULL;
	fun->tail = NULL;
}

void
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
