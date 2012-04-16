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

/* Output */
void dump_operand(struct operand*);
void dump_instruction(struct instr*);

#endif
