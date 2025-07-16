#ifndef INSTR
#error "INSTR not defined"
#endif

INSTR(INEG,		ineg)
INSTR(IADD,		iadd)
INSTR(ISUB,		isub)
INSTR(IMUL,		imul)
INSTR(IDIV,		idiv)
INSTR(IREM,		irem)
INSTR(IGT,		igt)
INSTR(ILT,		ilt)
INSTR(IGE,		ige)
INSTR(ILE,		ile)
INSTR(IEQ,		ieq)
INSTR(INE,		ine)

INSTR(FNEG,		fneg)
INSTR(FADD,		fadd)
INSTR(FSUB,		fsub)
INSTR(FMUL,		fmul)
INSTR(FDIV,		fdiv)
INSTR(FREM,		frem)
INSTR(FGT,		fgt)
INSTR(FLT,		flt)
INSTR(FGE,		fge)
INSTR(FLE,		fle)
INSTR(FEQ,		feq)
INSTR(FNE,		fne)

INSTR(NOT,		not)
INSTR(SHR,		shr)
INSTR(SHL,		shl)
INSTR(AND,		and)
INSTR(OR,		or)
INSTR(XOR,		xor)

INSTR(LOAD,		load)
INSTR(STORE,	store)
INSTR(ALLOC,	alloc)

INSTR(ELEMOF,	elemof)
INSTR(ADDROF,	addrof)

INSTR(BR,		br)
INSTR(RET,		ret)
INSTR(CALL,		call)
