#ifndef TAG
#error "TAG not defined"
#endif

#ifdef TYPE
#ifndef BASE
TAG(FUN,	fun,	TYPE)
TAG(TUPLE,	tuple,	TYPE)
TAG(IDENT,	ident,	TYPE)
TAG(NONE,	none,	TYPE)
TAG(VAR,	var,	TYPE)
#endif

TAG(UNIT,	unit,	TYPE)
TAG(INT,	int,	TYPE)
TAG(FLOAT,	float,	TYPE)
TAG(STR,	str,	TYPE)
TAG(CHAR,	char,	TYPE)
TAG(BOOL,	bool,	TYPE)
#endif
