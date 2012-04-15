/*
 * das things to do with instructions and opera
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include "common.h"

struct operand {
	int indirect;
	int reg;
	struct expr *expr;
	int known_word_count;
	u16 firstbits;
	u16 nextbits;
};

/* all parsed instructions in order encountered */
LIST_HEAD(instructions);

/*
 * Parse phase support
 */

/* generate an operand from reg (maybe -1), expr (maybe null), indirect flag */
struct operand* gen_operand(int reg, struct expr *expr, int indirect)
{
	//printf("gen_operand(reg:%d expr:%p %sdirect)\n", reg, expr, indirect ? "in":"");
	struct operand *o = calloc(1, sizeof *o);
	o->indirect = indirect;
	o->reg = reg;
	o->expr = expr;
	o->known_word_count = 0;
	return o;
}

/* generate an instruction from an opcode and one or two values */
void gen_instruction(int opcode, struct operand *a, struct operand *b)
{
	struct instr* i = malloc(sizeof *i);
	i->opcode = opcode;
	i->a = a;
	i->b = b;
	//printf("add instruction %p to list\n", i);
	list_add_tail(&i->list, &instructions);
}

/*
 *	Analysis
 */

/* (calculate and) get the word count needed to represent a value */
int operand_word_count(struct operand *o)
{
	int words;
	int maychange = 0;

	/* already calculated, and is a fixed size? */
	if (o->known_word_count > 0)
		return o->known_word_count;

	if (o->expr) {
		if (o->indirect) {
			/* all indirect forms use next-word */
			words = 2;
		} else {
			/* literal, is it small? */
			if (expr_value(o->expr) < 0x20) {
				/* small.. at least for now */
				words = 1;
				maychange = expr_contains_symbol(o->expr);
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
	int words = 1;	/* correctness check */

	// FIXME test this actually works
	if (0 == o->known_word_count) {
		/* symbol resolution has finished, size really is fixed now */
		o->known_word_count = operand_word_count(o);
	}

	if (o->expr)
		exprval = expr_value(o->expr);

	if (o->reg != -1)
		o->firstbits = reg2bits(o->reg);
	else
		o->firstbits = 0;

	if (o->firstbits & 0x10) {			// magic numbers ahoy!
		/* x-reg, bits set already */
		ERR_ON(o->expr);
	} else if (o->reg < 0) {
		/* no reg. have a expr. */
		if (o->indirect || exprval > 0x1f) {
			if (o->indirect) {
				/* [next word] form */
				o->firstbits = 0x1e;
			} else {
				/* next word literal */
				o->firstbits = 0x1f;
			}
			o->nextbits = (u16)exprval;
			words = 2;
		} else {
			/* small literal */
			o->firstbits = 0x20 | (u16)exprval;
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

	ERR_ON(words != o->known_word_count); // if true, assembler bug

	if (words == 2)
		o->nextbits = (u16)exprval;
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
		return !expr_contains_symbol(o->expr);
	}
	//printf("operand not fixed len\n");
	return 0;
}

/*
 * get instruction length. final length may increase due to symbol value
 * changes (but guaranteed length will never decrease)
 */
int instr_length(struct instr *i) {
	int len;

	/* shortcut if entirely static */
	if (i->length_known > 0) {
		//printf("shortcut: length known: %d\n", i->length_known);
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

/* write instruction binary to binfile */
/*
 * TODO make this go away when statements become more generic to accomodate
 * data statements etc.
 */
void instruction_fwritebin(FILE *f, struct instr *i)
{
	u16 word = 0;

	assert(i->a);
	operand_genbits(i->a);
	if (i->b)
		operand_genbits(i->b);

	if (i->opcode == 0) {
		/* special case JSR */
		word |= 1 << 4;
		word |= operand_firstbits(i->a) << 10;
	} else {
		assert(i->b);
		assert(i->opcode > 0 && i->opcode < 16);
		word |= i->opcode;
		word |= operand_firstbits(i->a) << 4;
		word |= operand_firstbits(i->b) << 10;
	}
	fwrite(&word, 2, 1, f);
	if (operand_needs_nextword(i->a)) {
		word = operand_nextbits(i->a);
		fwrite(&word, 2, 1, f);
	}
	if (i->b && operand_needs_nextword(i->b)) {
		word = operand_nextbits(i->b);
		fwrite(&word, 2, 1, f);
	}
}

void dump_operand(struct operand *o)
{
	assert(o);
	if (o->indirect)
		printf("[");
	if (o->expr)
		dump_expr(o->expr);
	if (o->expr && o->reg != -1)
		printf(" + ");
	if (o->reg != -1)
		printf("%s", reg2str(o->reg));
	if (o->indirect)
		printf("]");
}

void dump_instruction(struct instr *i)
{
	printf("%s ", opcode2str(i->opcode));
	dump_operand(i->a);
	if (i->b) {
		printf(", ");
		dump_operand(i->b);
	}
	printf("\tlength known: %d\n", i->length_known);
}

void dump_instructions(void)
{
	struct instr *i;
	printf("Dump instructions:\n");
	list_for_each_entry(i, &instructions, list) {
		printf("\t");
		dump_instruction(i);
	}
}
