#ifndef NODE
#error "NODE not defined"
#endif

#ifdef EXPR
NODE(IF,		If,		EXPR)
NODE(MATCH,		Match,	EXPR)
NODE(LET,		Let,	EXPR)
NODE(FUN,		Fun,	EXPR)
NODE(LAMBDA,	Lambda,	EXPR)
NODE(APP,		App,	EXPR)
NODE(IDENT,		Ident,	EXPR)
NODE(BINARY,	Binary,	EXPR)
NODE(TUPLE,		Tuple,	EXPR)
NODE(CONST,		Const,	EXPR)
#endif

#ifdef SIG
NODE(TYPE,		Type,	SIG)
#endif

#ifdef PATN
NODE(IDENT,		Ident,	PATN)
NODE(TUPLE,		Tuple,	PATN)
NODE(CONST,		Const,	PATN)
#endif
