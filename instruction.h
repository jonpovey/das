#ifndef INSTRUCTION_H
#define INSTRUCTION_H
/*
 * das things to do with instructions and operands
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
struct operand;
struct instr;

/* Parse */
struct operand* gen_operand(int reg, struct expr*, int indirect);
void gen_instruction(int opcode, struct operand *a, struct operand *b);

/* Analysis */
int instr_length(struct instr *i);

/* Output */
void dump_operand(struct operand*);
void dump_instruction(struct instr*);
void dump_instructions(void);

/* temp hack */
void instruction_fwritebin(FILE *f, struct instr *i);

/* things that shouldn't be exposed, hack hack for now */
#include "list.h"
extern struct list_head instructions;

struct instr {
	int opcode;
	struct operand* a;
	struct operand* b;
	int length_known;			/* 0 if unknown (maybe depends on symbols) */
	struct list_head list;		/* for master list of instructions */
};

#endif
