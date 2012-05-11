/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dasdefs.h"
#include "output.h"

/*
 * macro magic. use GCC designated initialisers to build a sparse array
 * of strings indexed by opcode. special opcodes are offset by 0x20
 */
#define OP(val, op, count) [val] = #op
#define SOP(val, op, count) [val | SPECIAL_OPCODE] = #op
static char *opcodes[64] = { OPCODES SPECIAL_OPCODES };
#undef OP
#undef SOP

/*
 * more macro magic. make a static array of struct reg which we can use for
 * looking up registers by string, and index into it to get associated data
 * such as binary value, general-purpose flag for validation, and string for
 * prettyprinting
 */
#define REGISTER(value, name, gp) { value, #name, gp }
static struct reg registers[] = { REGISTERS };
#undef REGISTER

int arrsearch(char *str, char **arr, int arrsize)
{
	int i;

	for (i = 0; i < arrsize; i++) {
		if (arr[i] && 0 == strcasecmp(str, arr[i]))
			return i;
	}
	fprintf(stderr, "BUG '%s' not found!\n", str);
	return -1;
}

inline int valid_reg(int reg)
{
	/* reg 0 is not valid, it means "no register" */
	return reg > 0 && reg < ARRAY_SIZE(registers);
}

inline int valid_opcode(int op)
{
	return op >= 0 && op < ARRAY_SIZE(opcodes) && opcodes[op];
}

/* get op value for an opcode string */
int str2opcode(char *str)
{
	return arrsearch(str, opcodes, ARRAY_SIZE(opcodes));
}

char* opcode2str(int op)
{
	assert(valid_opcode(op));
	return opcodes[op];
}

u16 opcode2bits(int opcode)
{
	assert(valid_opcode(opcode));
	if (is_special(opcode)) {
		/* will need << 5 to make into valid machine code */
		return opcode & ~SPECIAL_OPCODE;
	} else {
		return opcode;
	}
}

int is_special(int opcode)
{
	if (BUG_ON(!valid_opcode(opcode)))
		return -1;
	return opcode & SPECIAL_OPCODE;
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
	if (valid_reg(reg)) {
		if (outopts.stack_style_sp) {
			switch (reg) {
			case REG_PUSH: return "[--SP]";
			case REG_POP:  return "[SP++]";
			case REG_PEEK: return "[SP]";
			case REG_PICK: return "SP";		// rest handled in operand
			}
		}
		return registers[reg].name;
	} else {
		return NULL;
	}
}

u16 reg2bits(int reg)
{
	assert(valid_reg(reg));
	return registers[reg].valbits;
}

int is_gpreg(int reg)
{
	assert(valid_reg(reg));
	return registers[reg].is_gp;
}

inline int isoctal(int c) { return c >= '0' && c <= '7'; }

/*
 * unescape C string into provided buffer, which must be large enough.
 * result will be max strlen(src) + 1 characters
 * return number of characters (there may be embedded NULLs)
 */
int unescape_c_string(const char *src, unsigned char *dest)
{
	int n = 0;
	unsigned char c;
	char tmphex[3];

	while (*src) {
		c = 0;
		if (*src != '\\') {
			*dest++ = *src++;
			n++;
			continue;
		}

		/* escape sequence? */
		src++;
		if (!*src) {
			/* end of string was a backslash by itself. How? */
			fprintf(stderr, "string ends in backslash?\n");
			*dest++ = '\\';
			n++;
			break;
		}

		/* yes, it's an escape sequence */
		if (*src >= '0' && *src <= '3') {
			/* maybe start octal sequence? */
			if (isoctal(src[1]) && isoctal(src[2])) {
				c = (src[0] - '0') << 6 |
					(src[1] - '0') << 3 |
					(src[2] - '0');
				src += 2;				/* one more will be added shortly */
			} else if (*src == '0') {
				/* not followed by octal, treat as plain NULL */
				c = 0;
			} else {
				/* bad octal escape, treat as character */
				c = *src;
			}
		} else if (*src == 'x' && isxdigit(src[1]) && isxdigit(src[2])) {
			/* it's hex */
			tmphex[0] = src[1];
			tmphex[1] = src[2];
			tmphex[2] = 0;
			c = (char)strtol(tmphex, NULL, 16);
			src += 2;		/* one more will be added shortly */
		} else {
			/* escape sequence, not hex or octal or \0 NULL */
			switch (*src) {
			case 'a': c = '\a'; break;
			case 'b': c = '\b'; break;
			case 'f': c = '\f'; break;
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case 'v': c = '\v'; break;
			default:
				/* this covers [\?'"] */
				c = *src;
			}
		}
		*dest++ = c;
		src++;
		n++;
	}
	*dest = 0;	// terminate in case we want to print it unescaped
	return n;
}

/* re-escape the string for printing */
int sprint_cstring(char *buf, const unsigned char *in, int bytes)
{
	char *start = buf;
	char hexdigit[] = "0123456789abcdef";
	char c;

	while (bytes--) {
		int esc = 1;
		
		switch (*in) {
		case '\"':
		case '\\':
			c = *in;	/* escape as themselves */
			break;
		case '\n': c = 'n'; break;
		case '\t': c = 't'; break;
		case '\r': c = 'r'; break;
		default:
			esc = 0;
		}

		if (esc) {
			*buf++ = '\\';
			*buf++ = c;
			in++;
			continue;
		}

		if (*in > 0x1f && *in < 0x7f) {
			/* printable 7-bit ASCII */
			*buf++ = *in++;
		} else if (!*in) {
			*buf++ = '\\';
			*buf++ = '0';
			in++;
		} else {
			*buf++ = '\\';
			*buf++ = 'x';
			*buf++ = hexdigit[*in >> 4];
			*buf++ = hexdigit[*in & 0xf];
			in++;
		}
	}
	return buf - start;
}
