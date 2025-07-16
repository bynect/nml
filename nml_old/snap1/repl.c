#include <stdio.h>
#include <string.h>

#include "lexer.h"

int
main()
{
	const char src[] = "(fun x -> x + 10)\n(* (**) (**) (*(*(**)a*)b*)hello *)";
	//const char src[] = { '\n', 'f', 'u', 'n', ' ', 'l', 'e', 't', '1' };
	Lexer lex = lexer_init(src, sizeof(src));

	Token tok;
	while (lexer_next(&lex, &tok))
	{
		printf("%d: %.*s\n", tok.type, tok.len, tok.src);
	}

	return 0;
}
