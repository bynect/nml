#ifndef NODE
#error "NODE not defined"
#endif

#ifdef EXPR
NODE(IF,		If,		EXPR)
NODE(LET,		Let,	EXPR)
NODE(FUN,		Fun,	EXPR)
NODE(APP,		App,	EXPR)
NODE(IDENT,		Ident,	EXPR)
NODE(UNARY,		Unary,	EXPR)
NODE(BINARY,	Binary, EXPR)
NODE(TUPLE,		Tuple,	EXPR)
NODE(CONST,		Const,	EXPR)
#endif
