#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "nml_vm.h"
#include "nml_box.h"

#define VM_FETCH()			(*pc++)
#define VM_REG()			(regs[VM_FETCH()])

#define VM_WORD()			(pc += 2, (uint16_t)((pc[-2] << 8) | pc[-1]))
#define VM_OFF()			(pc += 4, (int32_t)((pc[-4] << 24) |(pc[-3] << 16) | (pc[-2] << 8) | pc[-1]))

#define VM_POP(type)		(stack -= sizeof(type), *(type *)(stack + sizeof(type)))
#define VM_PUSH(val, type)	(*((type *)stack) = val, stack += sizeof(type))

#ifdef MAC_COMPUTED_GOTO
#define VM_INSTR(instr)		INSTR_ ## instr:

#ifdef MAC_SHARED
#define VM_DISPATCH()		goto *(instr_base + instr_table[VM_FETCH()])
#else
#define VM_DISPATCH()		goto *instr_table[VM_FETCH()]
#endif

#define VM_START()			VM_DISPATCH();
#define VM_END()
#else
#define VM_INSTR(instr)		case INSTR_ ## instr:
#define VM_DISPATCH()		continue

#define VM_START()										\
	while (true)										\
	{													\
		switch (VM_FETCH())								\
		{
#define VM_END()										\
			default: 									\
				break;									\
		}												\
		abort();										\
	}
#endif

#define VM_IBIN_OF(op, type)							\
	Register reg = VM_FETCH();							\
	register int32_t rhs = VAL_UNINT(regs[reg]);		\
	register int32_t lhs = VAL_UNINT(VM_REG());			\
														\
	register int64_t n = rhs op lhs;					\
	if (MAC_UNLIKELY(n > INT32_MAX || n < INT32_MIN))	\
	{													\
		n = 0;											\
	}													\
	regs[reg] = type((int32_t)n);

#define VM_IBIN(op, type)								\
	Register reg = VM_FETCH();							\
	register const int32_t rhs = VAL_UNINT(regs[reg]);	\
	register const int32_t lhs = VAL_UNINT(VM_REG());	\
	regs[reg] = type(rhs op lhs)

#define VM_FBIN(op, type)								\
	Register reg = VM_FETCH();							\
	register const double rhs = VAL_UNFLOAT(regs[reg]);	\
	register const double lhs = VAL_UNFLOAT(VM_REG());	\
	regs[reg] = type(rhs op lhs)

void
vm_init(Virtual_Machine *vm)
{
	memset(vm->stack, 0, sizeof(vm->stack));
}

MAC_HOT Value
vm_eval(Virtual_Machine *vm, const uint8_t * code)
{
#ifdef MAC_COMPUTED_GOTO
#ifdef MAC_SHARED
	const void *const instr_base = &&INSTR_HLT;
	const uint32_t instr_table[] = {
#define INSTR(name, _)		[INSTR_ ## name] = &&INSTR_ ## name - instr_base,
#define BC
#include "nml_instr.h"
#undef BC
#undef INSTR
	};

	MAC_PREFETCH_CACHE(instr_base, 0);
#else
	const void *const instr_table[] = {
#define INSTR(name, _)		[INSTR_ ## name] = &&INSTR_ ## name,
#define BC
#include "nml_instr.h"
#undef BC
#undef INSTR
	};
#endif
#endif

	register uint8_t *stack = vm->stack;
	register const uint8_t *pc = code;
	Value regs[REG_MAX];

	MAC_PREFETCH_CACHE(pc, 0);
	MAC_PREFETCH_CACHE(regs, 1);

	VM_START();

	VM_INSTR(HLT)
	{
		return regs[REG_C0];
	}

	VM_INSTR(TRAP)
	{
		abort();
	}

	VM_INSTR(MOV)
	{
		Register reg = VM_FETCH();
		VM_REG() = regs[reg];
		VM_DISPATCH();
	}

	VM_INSTR(GEP)
	{
		Register reg = VM_FETCH();
		int32_t off = VM_OFF();
		Box_Tuple *tuple = (Box_Tuple *)VAL_UNBOX(regs[reg]);
		regs[reg] = tuple->items[off];
		VM_DISPATCH();
	}

	VM_INSTR(LD)
	{
		Register reg = VM_FETCH();
		int32_t off = VM_OFF();
		memcpy(&regs[reg], pc + off, sizeof(Value));
		VM_DISPATCH();
	}

	VM_INSTR(LDS)
	{
		Register reg = VM_FETCH();
		int32_t off = VM_OFF();
		regs[reg] = *(Value *)(stack + off);
		VM_DISPATCH();
	}

	VM_INSTR(ST)
	{
		Register reg = VM_FETCH();
		int32_t off = VM_OFF();
		memcpy(stack + off, &regs[reg], sizeof(Value));
		VM_DISPATCH();
	}

	VM_INSTR(POP)
	{
		VM_REG() = VM_POP(Value);
		VM_DISPATCH();
	}

	VM_INSTR(PUSH)
	{
		VM_PUSH(VM_REG(), Value);
		VM_DISPATCH();
	}

	VM_INSTR(ENTER)
	{
		uint16_t off = VM_WORD();
		VM_PUSH(stack, uint8_t *);
		stack += off * sizeof(Value);
		VM_DISPATCH();
	}

	VM_INSTR(LEAVE)
	{
		stack = VM_POP(uint8_t *);
		VM_DISPATCH();
	}

	VM_INSTR(IADD)
	{
		VM_IBIN_OF(+, VAL_INT);
		VM_DISPATCH();
	}

	VM_INSTR(ISUB)
	{
		VM_IBIN_OF(-, VAL_INT);
		VM_DISPATCH();
	}

	VM_INSTR(IMUL)
	{
		VM_IBIN_OF(*, VAL_INT);
		VM_DISPATCH();
	}

	VM_INSTR(IDIV)
	{
		Register reg = VM_FETCH();
		register int32_t rhs = VAL_UNINT(regs[reg]);
		register int32_t lhs = VAL_UNINT(VM_REG());

		if (MAC_UNLIKELY(lhs == 0))
		{
			abort();
		}

		register int64_t n = rhs / lhs;
		if (MAC_UNLIKELY(n > INT32_MAX || n < INT32_MIN))
		{
			//overflow
			n = 0;
		}
		regs[reg] = VAL_INT((int32_t)n);
		VM_DISPATCH();
	}

	VM_INSTR(IREM)
	{
		Register reg = VM_FETCH();
		register int32_t rhs = VAL_UNINT(regs[reg]);
		register int32_t lhs = VAL_UNINT(VM_REG());

		if (MAC_UNLIKELY(lhs == 0))
		{
			abort();
		}

		register int64_t n = rhs % lhs;
		if (MAC_UNLIKELY(n > INT32_MAX || n < INT32_MIN))
		{
			//overflow
			n = 0;
		}
		regs[reg] = VAL_INT((int32_t)n);
		VM_DISPATCH();
	}

	VM_INSTR(INEG)
	{
		Register reg = VM_FETCH();
		regs[reg] ^= VAL_MASK(31);
		VM_DISPATCH();
	}

	VM_INSTR(FADD)
	{
		VM_FBIN(+, VAL_FLOAT);
		VM_DISPATCH();
	}

	VM_INSTR(FSUB)
	{
		VM_FBIN(-, VAL_FLOAT);
		VM_DISPATCH();
	}

	VM_INSTR(FMUL)
	{
		VM_FBIN(*, VAL_FLOAT);
		VM_DISPATCH();
	}

	VM_INSTR(FDIV)
	{
		Register reg = VM_FETCH();
		register const double rhs = VAL_UNFLOAT(regs[reg]);
		register const double lhs = VAL_UNFLOAT(VM_REG());

		if (MAC_UNLIKELY(lhs == 0))
		{
			abort();
		}

		regs[reg] = VAL_FLOAT(rhs / lhs);
		VM_DISPATCH();
	}

	VM_INSTR(FNEG)
	{
		Register reg = VM_FETCH();
		regs[reg] ^= VAL_SIGNMASK;
		VM_DISPATCH();
	}

	VM_INSTR(IGT)
	{
		VM_IBIN(>, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(ILT)
	{
		VM_IBIN(<, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(IGE)
	{
		VM_IBIN(>=, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(ILE)
	{
		VM_IBIN(<=, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(FGT)
	{
		VM_FBIN(>, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(FLT)
	{
		VM_FBIN(<, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(FGE)
	{
		VM_FBIN(>=, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(FLE)
	{
		VM_FBIN(<=, VAL_BOOL);
		VM_DISPATCH();
	}

	VM_INSTR(EQ)
	{
		Register reg = VM_FETCH();
		register const Value rhs = regs[reg];
		register const Value lhs = VM_REG();
		regs[reg] = VAL_BOOL(rhs == lhs);
		VM_DISPATCH();
	}

	VM_INSTR(NE)
	{
		Register reg = VM_FETCH();
		register const Value rhs = regs[reg];
		register const Value lhs = VM_REG();
		regs[reg] = VAL_BOOL(rhs != lhs);
		VM_DISPATCH();
	}

	VM_INSTR(JMP)
	{
		int32_t off = VM_OFF();
		pc += off;
		VM_DISPATCH();
	}

	VM_INSTR(BR)
	{
		Register reg = VM_FETCH();
		int32_t off = VM_OFF();

		if (VAL_ISFALSE(regs[reg]))
		{
			pc += off;
		}
		VM_DISPATCH();
	}

	VM_INSTR(CALL)
	{
		int32_t off = VM_OFF();
		VM_PUSH(pc, const uint8_t *);
		pc += off;
		VM_DISPATCH();
	}

	VM_INSTR(CALLS)
	{
		Box_Fun *fun = (Box_Fun *)VAL_UNBOX(VM_REG());
		uint16_t off = VM_WORD();

		if (fun->kind == FUN_NATIVE)
		{
			regs[REG_C0] = fun->fun.ptr((Value *)stack);
			stack -= off * sizeof(Value);
		}
		else
		{
			VM_PUSH(pc, const uint8_t *);
			pc = fun->fun.bc.code;
		}
		VM_DISPATCH();
	}

	VM_INSTR(RET)
	{
		pc = VM_POP(const uint8_t *);
		VM_DISPATCH();
	}

	VM_END();
}
