/*
 * das things to do with instructions and operands
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#ifndef DEBUG_INSTRUCTION
  #undef DEBUG
#endif

#include <assert.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include "dasdefs.h"
#include "instruction.h"
#include "output.h"
#include "statement.h"

struct operand {
	enum opstyle style;
	enum op_pos position;
	int indirect;
	int reg;
	struct expr *expr;
	int known_word_count;
	u16 firstbits;
	u16 nextbits;
};

struct instr {
	int opcode;
	struct operand* a;
	struct operand* b;
	int length_known;		/* 0 if unknown (maybe depends on symbols) */
};

#define MAX_SHORT_LITERAL	0x1e
#define MIN_SHORT_LITERAL	-1

#define BINGEN_DBG(fmt, args...) (void)0
#define BINGEN_DBG_FUNC(fmt, args...) (void)0

/*
 * Parse phase support
 */
static struct statement_ops instruction_statement_ops;

/*
 * mask a constant value into *bits for writing.
 * return 0 if value fit in range, -1 or 1 if it wrapped round
 */
int mask_constant(int value, s16 *bits)
{
	*bits = (s16)value;
	if (value < -0x8000)
		return -1;
	else if (value > 0xffff)
		return 1;
	else
		return 0;
}

/* do some parse-time validation checks on an operand */
void operand_validate(struct operand *o)
{
	DBG_FUNC("pos:%s style:%d indirect:%d reg:%d(%s) expr:%p\n",
		o->position == OP_POS_A ? "a" : "b",
		o->style, o->indirect, o->reg, reg2str(o->reg), o->expr);
	/*
	 * general-purpose and special regs are combined now by the parser,
	 * check for shennanigans like [PC + 3] or bare X + 1
	 */
	if (o->style == OPSTYLE_PLUS && ! o->indirect) {
		error("Register + constant illegal outside [brackets]");
	}
	if (o->indirect && o->reg) {
		/* only general-purpose registers and SP allowed (solo or + const) */
		if (is_gpreg(o->reg) || o->reg == REG_SP) {
			// OK
		} else {
			error("Can't use %s inside [brackets]", reg2str(o->reg));
		}
	}

	// check PICK style is paired up with PICK register
	if (o->style == OPSTYLE_PICK && o->reg != REG_PICK) {
		error("'Register value' form only permitted for PICK");
	}
	if (o->reg == REG_PICK && o->style != OPSTYLE_PICK) {
		if (o->style == OPSTYLE_SOLO) {
			error("PICK without offset");
		} else if (o->style == OPSTYLE_PLUS) {
			/* could warn and allow this? */
			error("Register PICK cannot be combined with '+'");
		} else {
			BUG();
		}
	}

	/* validate a/b specifics */
	if (o->position == OP_POS_A) {
		if (o->reg == REG_PUSH) {
			error("PUSH not usable as source ('a') operand");
		}
	} else if (o->position == OP_POS_B) {
		if (o->reg == REG_POP) {
			error("POP not usable as destination ('b') operand");
		}
		if (o->expr && !o->reg && !o->indirect) {
			/*
			 * literal destination will be ignored, warn. Maybe has valid use
			 * for arithmetic, EX things?
			 */
			warn("Literal value used as destination");
		}
	} else {
		BUG();
	}

}

/* generate an operand from reg (maybe -1), expr (maybe null), style (type) */
struct operand* gen_operand(int reg, struct expr *expr, enum opstyle style)
{
	TRACE1("reg:%d expr:%p style:%i\n", reg, expr, style);
	struct operand *o = calloc(1, sizeof *o);
	o->style = style;
	o->reg = reg;
	o->expr = expr;
	o->known_word_count = 0;
	return o;
}

struct operand* operand_set_indirect(struct operand *o)
{
	assert(o);
	BUG_ON(o->indirect);
	o->indirect = 1;
	return o;
}

/*
 * need to know if operand is in a or b position for PUSH/POP validation,
 * b literal warnings, and b short literal disable.
 */
struct operand* operand_set_position(struct operand *o, enum op_pos pos)
{
	assert(pos == OP_POS_A || pos == OP_POS_B);
	BUG_ON(o->position != 0);
	o->position = pos;
	return o;
}

/* generate an instruction from an opcode and one or two values */
void gen_instruction(int opcode, struct operand *b, struct operand *a)
{
	struct instr* i = calloc(1, sizeof *i);
	i->opcode = opcode;
	i->a = a;
	i->b = b;
	/* do validation at some later stage? */
	if (a) {
		operand_validate(a);
	}
	if (b) {
		operand_validate(b);
	}
	//DBG("add instruction %p to list\n", i);
	add_statement(i, &instruction_statement_ops);
}

/*
 *	Analysis
 */

/* (calculate and) get the word count needed to represent a value */
int operand_word_count(struct operand *o)
{
	int words, value;
	int maychange = 0;

	/* already calculated, and is a fixed size? */
	if (o->known_word_count > 0)
		return o->known_word_count;

	if (o->expr) {
		if (o->indirect || o->reg == REG_PICK || o->position == OP_POS_B) {
			/* all indirect forms use next-word, and b can't be short literal */
			words = 2;
		} else {
			s16 valbits;
			/*
			 * literal, is it short?
			 * need to mask value to check, e.g. 0x1ffff would mask (with
			 * a warning) to 0xffff and therefore be -1, short form
			 */
			value = expr_value(o->expr);
			mask_constant(value, &valbits);
			DBG_FUNC("value:%d bits:%d\n", value, valbits);
			if (valbits <= MAX_SHORT_LITERAL && valbits >= MIN_SHORT_LITERAL) {
				/* short.. at least for now */
				words = 1;
				maychange = expr_maychange(o->expr);
			} else {
				words = 2;
			}
		}
	} else {
		/* no expr, next-word never used */
		words = 1;
	}

	/*
	 * instructions are never allowed to become smaller, to ward off
	 * infinite loops. If words == 2, maychange is not set.
	 */
	if (!maychange) {
		o->known_word_count = words;
	} else {
		//printf("DBG: this operand length may change!\n");
	}

	return words;
}

int operand_needs_nextword(struct operand *o)
{
	return operand_word_count(o) - 1;
}

/* validate completeness and generate bits for operand */
void operand_genbits(struct operand *o)
{
	int exprval;
	s16 valword;
	int words = 1;	/* correctness check */
	int ret;

	// FIXME test this actually works
	if (0 == o->known_word_count) {
		/* symbol resolution has finished, size really is fixed now */
		o->known_word_count = operand_word_count(o);
	}

	if (o->expr) {
		exprval = expr_value(o->expr);
		ret = mask_constant(exprval, &valword);
		if (ret) {
			warn("Value %d(0x%x) does not fit in 16 bits, masked to 0x%x",
				exprval, exprval, valword);
		}
	}

	if (o->reg)
		o->firstbits = reg2bits(o->reg);
	else
		o->firstbits = 0;

	if (o->reg && !is_gpreg(o->reg)) {
		/* x-reg, firstbits are good */
		if (o->reg == REG_PICK) {
			/* PICK needs nextword */
			assert(o->expr);
			words = 2;
		}
	} else if (!o->reg) {
		/* no reg. have a expr. */
		if (o->indirect) {
			/* [next word] form */
			o->firstbits = 0x1e;
			words = 2;
		} else if (o->known_word_count == 1) {
			/* small literal */
			BUG_ON(o->position == OP_POS_B);
			BUG_ON(valword > MAX_SHORT_LITERAL);
			BUG_ON(valword < MIN_SHORT_LITERAL);
			o->firstbits = 0x20 | (valword - MIN_SHORT_LITERAL);
		} else {
			/* next word literal */
			o->firstbits = 0x1f;
			words = 2;
		}
	} else {
		/* gpreg in some form */
		if (o->indirect) {
			if (o->expr) {
				/* [next word + register] */
				o->firstbits |= 0x10;
				words = 2;
			} else {
				/* [gp register] */
				o->firstbits |= 0x08;
			}
		}
		/* else plain gpreg, nothing to modify */
	}

	BUG_ON(words != o->known_word_count); // if true, assembler bug

	if (words == 2)
		o->nextbits = valword;
}

u16 operand_firstbits(struct operand *o)
{
	assert(o->known_word_count >= 1);
	//printf("firstbits: 0x%x\n", o->firstbits);
	return o->firstbits;
}

u16 operand_nextbits(struct operand *o)
{
	assert(o->known_word_count == 2);
	//printf("nextbits: 0x%x\n", o->nextbits);
	return o->nextbits;
}

/* return true if operand binary length is known */
int operand_fixed_len(struct operand *o)
{
	if (o->known_word_count > 0) {
		//printf("fixed: known words\n");
		return 1;
	}

	/* only false if symbol-based literal. */
	if (!o->indirect && o->expr) {
		/* literal */
		return !expr_maychange(o->expr);
	}
	//printf("operand not fixed len\n");
	return 0;
}

/*
 * get instruction length. final length may increase due to symbol value
 * changes (but guaranteed length will never decrease)
 */
static int instruction_binary_size(void *private) {
	int len;
	struct instr *i = private;

	/* shortcut if entirely static */
	if (i->length_known > 0) {
		TRACE2("shortcut: length known: %d\n", i->length_known);
		return i->length_known;
	}

	len = 1 + operand_needs_nextword(i->a);
	if (i->b)
		len += operand_needs_nextword(i->b);

	if (operand_fixed_len(i->a) && (!i->b || operand_fixed_len(i->b))) {
		/* not going to change, can shortcut next time */
		//printf("set shortcut: %d\n", len);
		i->length_known = len;
	}
	return len;
}

/*
 * Output support
 */

int instruction_get_binary(u16 *dest, void *private)
{
	u16 word = 0;
	struct instr *i = private;
	int nwords = 0;

	assert(i->a);
	operand_genbits(i->a);
	BINGEN_DBG_FUNC("opcode:%x", i->opcode);
	BINGEN_DBG(" a:%x", operand_firstbits(i->a));
	if (i->b) {
		operand_genbits(i->b);
		BINGEN_DBG(" b:%x", operand_firstbits(i->b));
	}

	if (is_special(i->opcode)) {
		BINGEN_DBG(" special");
		word |= opcode2bits(i->opcode) << 5;
		BINGEN_DBG(" w/op:%x ", word);
		word |= operand_firstbits(i->a) << 10;
	} else {
		assert(i->b);
		BINGEN_DBG(" normal");
		word |= opcode2bits(i->opcode);
		BINGEN_DBG(" w/op:%04x", word);
		word |= operand_firstbits(i->a) << 10;
		BINGEN_DBG(" w/a:%x", word);
		/* b may not be short literal form */
		word |= operand_firstbits(i->b) << 5;
	}
	BINGEN_DBG("\n");
	dest[nwords++] = word;
	if (operand_needs_nextword(i->a)) {
		dest[nwords++] = operand_nextbits(i->a);
	}
	if (i->b && operand_needs_nextword(i->b)) {
		dest[nwords++] = operand_nextbits(i->b);
	}
	return nwords;
}

int operand_print_asm(char *buf, struct operand *o)
{
	int count = 0;

	assert(o);
	if (o->style == OPSTYLE_PICK) {
		assert(o->reg == REG_PICK);
		assert(o->expr);
		/* can't be indirect */
		count += sprintf(buf + count, "%s ", reg2str(o->reg));
		count += expr_print_asm(buf + count, o->expr);
	} else {
		if (o->indirect)
			count += sprintf(buf + count, "[");
		if (o->expr)
			count += expr_print_asm(buf + count, o->expr);
		if (o->expr && o->reg)
			count += sprintf(buf + count, " + ");
		if (o->reg)
			count += sprintf(buf + count, "%s", reg2str(o->reg));
		if (o->indirect)
			count += sprintf(buf + count, "]");
	}
	return count;
}

static int instruction_print_asm(char *buf, void *private)
{
	int count;
	struct instr *i = private;

	count = sprintf(buf, "%s ", opcode2str(i->opcode));
	if (i->b) {
		count += operand_print_asm(buf + count, i->b);
		count += sprintf(buf + count, ", ");
	}
	count += operand_print_asm(buf + count, i->a);
	return count;
}

/* cleanup */
static void free_operand(struct operand *o)
{
	if (o->expr)
		free_expr(o->expr);
	free(o);
}

static void instruction_free_private(void *private)
{
	struct instr *i = private;
	if (i->a)
		free_operand(i->a);
	if (i->b)
		free_operand(i->b);
	free(i);
}

static struct statement_ops instruction_statement_ops = {
	.analyse         = NULL,	/* all done during get-length.. for now */
	.get_binary_size = instruction_binary_size,
	.get_binary      = instruction_get_binary,
	.print_asm       = instruction_print_asm,
	.free_private    = instruction_free_private,
	.type            = STMT_INSTRUCTION,
};
