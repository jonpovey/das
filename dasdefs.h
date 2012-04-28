/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#ifndef DASDEFS_H
#define DASDEFS_H

typedef unsigned short u16;

struct reg {
	char *name;
	u16  valbits;
};

int str2opcode(char *str);
char* opcode2str(int op);

int str2reg(char *str);
char* reg2str(int reg);
u16 reg2bits(int reg);

void yyerror(char *);

int unescape_c_string(const char *src, unsigned char *dest);
int sprint_cstring(char *buf, const unsigned char *str, int bytes);

#define ERR_ON(x) ({ int r = !!(x); \
	if (r) { \
		fprintf(stderr, "%s:%d ERR_ON(%s)\n", __FILE__, __LINE__, #x); \
		das_error = 1; \
	} \
	r; \
})

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif // #ifndef ARRAY_SIZE

/* Opcodes: OP(opcode value, name, minimum cycle count) */
#define OPCODES \
	OP(0x01, SET, 1), \
	OP(0x02, ADD, 2), \
	OP(0x03, SUB, 3), \
	OP(0x04, MUL, 2), \
	OP(0x05, MLI, 2), \
	OP(0x06, DIV, 3), \
	OP(0x07, DVI, 3), \
	OP(0x08, MOD, 3), \
	OP(0x09, MDI, 3), \
	OP(0x0a, AND, 1), \
	OP(0x0b, BOR, 1), \
	OP(0x0c, XOR, 1), \
	OP(0x0d, SHR, 2), \
	OP(0x0e, ASR, 2), \
	OP(0x0f, SHL, 2), \
	OP(0x10, IFB, 2), \
	OP(0x11, IFC, 2), \
	OP(0x12, IFE, 2), \
	OP(0x13, IFN, 2), \
	OP(0x14, IFG, 2), \
	OP(0x15, IFA, 2), \
	OP(0x16, IFL, 2), \
	OP(0x17, IFU, 2), \
	OP(0x1a, ADX, 3), \
	OP(0x1b, SBX, 3), \
	OP(0x1e, STI, 2), \
	OP(0x1f, STD, 2),

#define SPECIAL_OPCODES \
	SOP(0x01, JSR, 3), \
	SOP(0x07, HCF, 9), /* undocumented since spec 1.7 */ \
	SOP(0x08, INT, 4), \
	SOP(0x09, IAG, 1), \
	SOP(0x0a, IAS, 1), \
	SOP(0x0b, RFI, 3), /* hooray! */ \
	SOP(0x0c, IAQ, 2), /* hooray! */ \
	SOP(0x10, HWN, 2), \
	SOP(0x11, HWQ, 4), \
	SOP(0x12, HWI, 4),

#define SPECIAL_OPCODE 0x20

#endif
