#ifndef TOK
#error "TOK not defined"
#endif

#ifdef SPEC
TOK(EOF,	"Eof",		eof,	SPEC)
TOK(ERR,	"Error",	err,	SPEC)
#endif

#ifdef EXTRA
TOK(AMP,  	"&",		_,		EXTRA)
TOK(COL,  	":",		_,		EXTRA)
TOK(HASH,  	"#",		_,		EXTRA)
TOK(BTICK,	"`",		_,		EXTRA)
TOK(ULINE,	"_",		_,		EXTRA)
#endif

#ifdef DELIM
TOK(LPAR, 	"(",		_,		DELIM)
TOK(RPAR, 	")",		_,		DELIM)
TOK(LBRACK,	"[",		_,		DELIM)
TOK(RBRACK,	"]",		_,		DELIM)
TOK(LBRACE,	"{",		_,		DELIM)
TOK(RBRACE,	"}",		_,		DELIM)
TOK(BAR,  	"|",		_,		DELIM)
#endif

#ifdef OPER
TOK(PLUS, 	"+",		_,		OPER)
TOK(MINUS,	"-",	 	_,		OPER)
TOK(STAR, 	"*",		_,		OPER)
TOK(SLASH,	"/",	 	_,		OPER)
TOK(PERC,	"%",	 	_,		OPER)
TOK(PLUSF, 	"+.",		_,		OPER)
TOK(MINUSF,	"-.",	 	_,		OPER)
TOK(STARF, 	"*.",		_,		OPER)
TOK(SLASHF,	"/.",	 	_,		OPER)
TOK(PERCF,	"%.",	 	_,		OPER)
TOK(CARET,	"^",	 	_,		OPER)
TOK(DOT,  	".",		_,		OPER)
TOK(AT,  	"@",		_,		OPER)
TOK(BANG,  	"!",		_,		OPER)
TOK(GT,   	">",		_,		OPER)
TOK(LT,   	"<",		_,		OPER)
TOK(GTE,  	">=",	  	_,		OPER)
TOK(LTE,  	"<=",	  	_,		OPER)
TOK(EQ,   	"=",		_,		OPER)
TOK(NE,   	"!=",		_,		OPER)
TOK(AMPAMP, "&&",	  	_,		OPER)
TOK(BARBAR, "||",	  	_,		OPER)
TOK(ARROW,	"->",	 	_,		OPER)
TOK(COLCOL, "::",		_,		OPER)
TOK(COMMA,	",",		_,		OPER)
TOK(SEMI,	";",		_,		OPER)
TOK(TILDE, 	"~",		_,		OPER)
#endif

#ifdef LIT
TOK(IDENT,	"Ident",	ident,	LIT)
TOK(INT, 	"Int",		int,	LIT)
TOK(FLOAT,	"Float",	float,	LIT)
TOK(STR,	"Str",		str,	LIT)
TOK(CHAR,	"Char",		char,	LIT)
#endif

#ifdef KWD
TOK(TRUE,	"true",		true,	KWD)
TOK(FALSE,	"false",	false,	KWD)
TOK(LET,	"let",		let,	KWD)
TOK(IN,		"in",		in,		KWD)
TOK(AND,	"and",		and,	KWD)
TOK(FUN,	"fun",		fun,	KWD)
TOK(IF,		"if",		if,		KWD)
TOK(THEN,	"then",		then,	KWD)
TOK(ELSE,	"else",		else,	KWD)
TOK(MATCH,	"match",	match,	KWD)
TOK(WITH,	"with",		with,	KWD)
TOK(TYPE,	"type",		type,	KWD)
#endif
