#ifndef TAG
#error "TAG not defined"
#endif

#ifdef TYPE
#ifndef PRIM
TAG(FUN,	Fun,	TYPE)
TAG(TUPLE,	Tuple,	TYPE)
TAG(VAR,	Var,	TYPE)
TAG(CONSTR,	Constr,	TYPE)
TAG(NONE,	None,	TYPE)
#endif

TAG(UNIT,	Unit,	TYPE)
TAG(INT,	Int,	TYPE)
TAG(FLOAT,	Float,	TYPE)
TAG(STR,	Str,	TYPE)
TAG(CHAR,	Char,	TYPE)
TAG(BOOL,	Bool,	TYPE)
#endif
