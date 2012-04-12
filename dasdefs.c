#include <assert.h>
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

static struct reg registers[] = {
	{ "a",    0 },
	{ "b",    1 },
	{ "c",    2 },
	{ "x",    3 },
	{ "y",    4 },
	{ "z",    5 },
	{ "i",    6 },
	{ "j",    7 },
	{ "POP",  0x18 },
	{ "PEEK", 0x19 },
	{ "PUSH", 0x1a },
	{ "SP",   0x1b },
	{ "PC",   0x1c },
	{ "O",    0x1d },
};

//static char *xregs[] = { "POP", "PEEK", "PUSH", "SP", "PC", "O" };

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

int str2reg(char *str)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(registers); i++) {
		if (0 == strcasecmp(str, registers[i].name))
			return i;
	}
	fprintf(stderr, "BUG reg '%s' not found!\n", str);
	return -1;
}

char* reg2str(int reg)
{
	if (reg >= 0 && reg < ARRAY_SIZE(registers)) {
		return registers[reg].name;
	} else {
		return NULL;
	}
}

u16 reg2bits(int reg)
{
	assert(reg >= 0 && reg < ARRAY_SIZE(registers));
	return registers[reg].valbits;
}
