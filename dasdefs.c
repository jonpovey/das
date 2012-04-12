#include "dasdefs.h"

static char *op2codes[] = {
	"non-basic",
	"SET",
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"MOD",
	"SHL",
	"SHR",
	"AND",
	"BOR",
	"XOR",
	"IFE",
	"IFN",
	"IFG",
	"IFB",
};


/* get opcode for an op2 string */
int op2code(char *str)
{
	int i;

	for (i = 1; i < ARRAY_SIZE(op2codes); i++) {
		if (0 == strcasecmp(str, op2codes[i]))
			return i;
	}
	yyerror("BUG unknown opcode!\n");
	return -1;
}
