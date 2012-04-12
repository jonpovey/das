#include <stdio.h>
#include <string.h>

#include "dasdefs.h"

static char *opcodes[] = {
	"JSR",	/* special case */
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

static char *xregs[] = { "SP", "PC", "O", "POP", "PEEK", "PUSH" };

int arrsearch(char *str, char **arr, int arrsize)
{
	int i;

	for (i = 0; i < arrsize; i++) {
		if (0 == strcasecmp(str, arr[i]))
			return i;
	}
	fprintf(stderr, "BUG '%s' not found!\n", str);
	return -1;
}

/* get op value for an opcode string */
int str2op(char *str)
{
	return arrsearch(str, opcodes, ARRAY_SIZE(opcodes));
}

char* op2str(int op)
{
	if (op >= 0 && op < ARRAY_SIZE(opcodes))
		return opcodes[op];
	else
		return "INVALID";
}

int str2xreg(char *str)
{
	return arrsearch(str, xregs, ARRAY_SIZE(xregs));
}

char* reg2str(int reg)
{
	static char hackstr[2];

	if (reg >= 0 && reg < ARRAY_SIZE(xregs)) {
		return xregs[reg];
	} else {
		hackstr[0] = (char)reg;
		hackstr[1] = 0;
		return hackstr;
	}
}
