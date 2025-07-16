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
NODE(CONSTR,	Constr,	EXPR)
NODE(TUPLE,		Tuple,	EXPR)
NODE(LIT,		Lit,	EXPR)
#endif

#ifdef DECL
NODE(LET,		Let,	DECL)
NODE(FUN,		Fun,	DECL)
NODE(DATA,		Data,	DECL)
NODE(TYPE,		Type,	DECL)
#endif

#ifdef PATN
NODE(IDENT,		Ident,	PATN)
NODE(AS,		As,		PATN)
NODE(CONSTR,	Constr,	PATN)
NODE(TUPLE,		Tuple,	PATN)
NODE(LIT,		Lit,	PATN)
#endif
