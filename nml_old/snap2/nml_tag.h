#ifndef TAG
#error "TAG not defined"
#endif

#ifdef TYPE
#ifndef PRIM
TAG(FUN,	fun,	TYPE)
TAG(TUPLE,	tuple,	TYPE)
TAG(NAME,	name,	TYPE)
TAG(CONS,	cons,	TYPE)
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

#ifdef BOX
TAG(FUN,	Fun,	BOX)
TAG(TUPLE,	Tuple,	BOX)
TAG(STR,	Str,	BOX)
#endif

#ifdef VALUE
TAG(UNIT,	unit,	VALUE)
TAG(INT,	int,	VALUE)
TAG(CHAR,	char,	VALUE)
TAG(TRUE,	true,	VALUE)
TAG(FALSE,	false,	VALUE)
#endif
