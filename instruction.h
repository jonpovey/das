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

enum opstyle {
	OPSTYLE_SOLO,
	OPSTYLE_PICK,
	OPSTYLE_PLUS,
};

/* Parse */
struct operand* gen_operand(int reg, struct expr*, enum opstyle style);
void gen_instruction(int opcode, struct operand *b, struct operand *a);
struct operand* operand_set_indirect(struct operand *);

/* Analysis */

/* Output */
void dump_operand(struct operand*);
void dump_instruction(struct instr*);

#endif
