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
	OPSTYLE_SOLO = 1,
	OPSTYLE_PICK,
	OPSTYLE_PLUS,
};

enum op_pos {
	OP_POS_A = 1,
	OP_POS_B,
};

/* Parse */
struct operand* gen_operand(int reg, struct expr*, enum opstyle style);
void gen_instruction(int opcode, struct operand *b, struct operand *a);
struct operand* operand_set_indirect(struct operand *);
struct operand* operand_set_position(struct operand *o, enum op_pos pos);

/* Analysis */

/* Output */
void dump_operand(struct operand*);
void dump_instruction(struct instr*);

#endif
