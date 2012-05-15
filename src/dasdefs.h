/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#ifndef DASDEFS_H
#define DASDEFS_H

typedef unsigned short u16;
typedef   signed short s16;

struct reg {
	u16  valbits;
	char *name;
	int is_gp;		/* general-purpose register [ABCXYZIJ] */
};

int str2opcode(char *str);
char* opcode2str(int op);
u16 opcode2bits(int opcode);
int is_special(int opcode);

int str2reg(char *str);
char* reg2str(int reg);
u16 reg2bits(int reg);
int is_gpreg(int reg);

void yyerror(char *s, ...);

int unescape_c_string(const char *src, unsigned char *dest);
int sprint_cstring(char *buf, const unsigned char *str, int bytes);

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

/*
 * registers that can appear in values
 * REGISTER(binary (base) value, name, gp_flag)
 * gp_flag means general-purpose register: can use [reg] and [reg + nextword]
 * forms by adding 0x8 / 0x10
 */
#define REGISTERS \
	REGISTER(0xff, NONE, 0), /* reserve zero to mean no register */ \
	REGISTER(0,    A,    1), \
	REGISTER(1,    B,    1), \
	REGISTER(2,    C,    1), \
	REGISTER(3,    X,    1), \
	REGISTER(4,    Y,    1), \
	REGISTER(5,    Z,    1), \
	REGISTER(6,    I,    1), \
	REGISTER(7,    J,    1), \
	REGISTER(0x18, PUSH, 0), \
	REGISTER(0x18, POP,  0), /* note PUSH, POP have same opcode */ \
	REGISTER(0x19, PEEK, 0), \
	REGISTER(0x1a, PICK, 0), \
	REGISTER(0x1b, SP,   0), \
	REGISTER(0x1c, PC,   0), \
	REGISTER(0x1d, EX,   0),

/* declare lookup enum of registers, will index into array */
#define REGISTER(val, name, gp) REG_##name
enum regs { REGISTERS };
#undef REGISTER

#endif
