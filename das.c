/* das! */
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "das.h"
#include "dasdefs.h"
#include "list.h"

int yyparse(void);
int das_error = 0;
FILE* binfile;

/*
symbol storage
-------------

store name string and some kind of reference for figuring out the address it
resolves to. When resolved, store an int?

During parsing, encoutering a label, associate that symbol with the preceding
statement (following statement not yet encountered). Instruction length not
known during parse time, or is it?

*/

// need forward decl for circular references
struct instr;

struct symbol {
	char *name;
	int  value;
	struct instr *after_instr;	/* defined at PC after this instruction */
	struct list_head list;		/* on undefined or defined list */
};

struct expr {
	int issymbol;
	union {
		int value;
		struct symbol *symbol;
	};
};

struct operand {
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
	int length_known;			/* 0 if unknown (maybe depends on symbols) */
	struct list_head list;		/* for master list of instructions */
};

/* all parsed instructions in order encountered */
static LIST_HEAD(instructions);

/*
 * symbol list management
 */
static LIST_HEAD(defined_symbols);	/* symbols go on this list when :def found */
static LIST_HEAD(undefined_symbols);	/* on this list if :def not found yet */

/* little helper function */
struct symbol* first_defined_symbol(void)
{
	if (list_empty(&defined_symbols))
		return NULL;
	else
		return list_entry(defined_symbols.next, struct symbol, list);
}

struct symbol* next_defined_symbol(struct symbol *symbol)
{
	if (symbol->list.next == &defined_symbols)
		return NULL; /* no more symbols */
	else
		return list_entry(symbol->list.next, struct symbol, list);
}

/* return symbol ptr if found (by name) in given list */
struct symbol* symbol_inlist(char *name, struct list_head *head)
{
	struct symbol *l;
	list_for_each_entry(l, head, list) {
		if (0 == strcmp(l->name, name))
			return l;
	}
	return NULL;
}

static void dump_symbol(struct symbol *l)
{
	printf("%s = 0x%x\n", l->name, l->value);
}

static void dump_symbols(void)
{
	struct symbol *symbol;

	printf("defined symbols:");
	if (list_empty(&defined_symbols)) {
		printf(" None\n");
	} else {
		printf("\n");
		list_for_each_entry(symbol, &defined_symbols, list)
			dump_symbol(symbol);
	}
	printf("undefined symbols:");
	if (list_empty(&undefined_symbols)) {
		printf(" None\n");
	} else {
		printf("\n");
		list_for_each_entry(symbol, &undefined_symbols, list)
			dump_symbol(symbol);
	}
}

/* parser encountered a symbol */
struct symbol* symbol_parse(char *name)
{
	// search symbol list (or hash, tree) for matching symbol
	struct symbol *l;
	l = symbol_inlist(name, &defined_symbols);
	if (!l)
		l = symbol_inlist(name, &undefined_symbols);
	if (!l) {
		/* not seen a symbol with this name before */
		l = calloc(1, sizeof *l);
		l->name = strdup(name);
		list_add_tail(&l->list, &undefined_symbols);
	}
	return l;
}

/*
 * parser encountered a label, (symbol definition).
 * called from parser, nothing to return, just note label location
 */
void label_parse(char *name)
{
	struct symbol *l;
	l = symbol_inlist(name, &defined_symbols);
	if (l) {
		/* error, multiple definition */
		fprintf(stderr, "error: symbol '%s' redefinition\n", name);
		/* FIXME bubble it to latest definition.. bad, but consistent */
		list_move_tail(&l->list, &defined_symbols);
	} else {
		l = symbol_inlist(name, &undefined_symbols);
		if (l) {
			/* move to defined symbols list */
			list_move_tail(&l->list, &defined_symbols);
		}
	}
	if (!l) {
		l = calloc(1, sizeof *l);
		l->name = strdup(name);
		list_add_tail(&l->list, &defined_symbols);
	}
	/*
	 * associate with latest instruction parsed.
	 * if no instructions yet, associate NULL. this is valid and means
	 * symbol value 0.
	 */
	if (list_empty(&instructions)) {
		//printf("associate label %s with NULL\n", name);
		l->after_instr = NULL;
		l->value = 0;
	} else {
		l->after_instr = list_entry(instructions.prev, struct instr, list);
		//printf("associate label %s with instruction %p\n", name,
		//		l->after_instr);
	}
}

/* generate a expr for a const */
struct expr* gen_const(int val)
{
	//printf("gen_const: %d\n", val);
	struct expr *e = malloc(sizeof *e);
	e->issymbol = 0;
	e->value = val;
	return e;
}

struct expr* gen_symbol(char *str)
{
	//printf("gen_symbol: %s\n", str);
	struct expr *e = malloc(sizeof *e);
	e->issymbol = 1;
	e->symbol = symbol_parse(str);
	return e;
}

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
 * dump functions
 */
void dump_expr(struct expr *e)
{
	if (e->issymbol) {
		printf("%s(0x%x)", e->symbol->name, e->symbol->value);
	} else {
		printf("0x%x", e->value);
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

/*
 * bit generation
 */
int expr_value(struct expr *expr)
{
	assert(expr);
	if (expr->issymbol) {
		return expr->symbol->value;
	} else {
		return expr->value;
	}
}

int expr_contains_symbol(struct expr *expr)
{
	//printf("contains symbol: %d\n", expr->issymbol);
	return expr->issymbol;
}

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

/* write instruction binary to binfile */
void writebin_instruction(struct instr *i)
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
	fwrite(&word, 2, 1, binfile);
	if (operand_needs_nextword(i->a)) {
		word = operand_nextbits(i->a);
		fwrite(&word, 2, 1, binfile);
	}
	if (i->b && operand_needs_nextword(i->b)) {
		word = operand_nextbits(i->b);
		fwrite(&word, 2, 1, binfile);
	}
}

/*
 * Walk the list of instructions.
 * Compute instruction size if known[1].
 * Maintain a running total of instruction sizes (PC value).
 *
 * Simultaneously walk the list of labels, which is also in memory order.
 * When a symbol matches the current instruction, set that symbol's value to
 * the current PC.
 *
 * [1] Instruction size may not be known with literals that contain symbols,
 * if we aren't yet sure of symbol value. In this case assume we can use a short
 * literal until symbol is known.
 *
 * Call this function more than once.
 * On the first pass, symbols defined later will have unknown value.
 * On the second pass, symbols will have a possibly-correct value and some
 * instruction lengths may increase[2] due to literals becoming too long and
 * requiring the next-word literal form. 
 *
 * Instructions can only become longer. Keep running this function until no
 * symbol values change (function returns 0).
 *
 * In a pathological case (needing compile-time constant expressions and a
 * deliberate attempt at breakage, I think), this "bubbling-up" may have to
 * run many times, but not more times than there are instructions.
 *
 * [2] If later I implement compile-time constant expressions, a symbol could
 * change value such that a literal became smaller, e.g. (0x40 - foo) if
 * foo changes from 0x20 to 0x21, the literal would become 0x1f and no longer
 * require a next-word.
 * To guard against a potential infinite loop I intend to not permit
 * literals to become smaller. If they ever need a next-word, set a
 * "using next-word" flag for them and even if the value reduces, still use
 * next-word form.
 *
 * I should test that handling. Yes, I am paranoid.
 *
 * Return value: number of symbols whose value changed on this run
 */
int walk_instructions(void)
{
	int symbols_changed = 0;
	int pc = 0;
	struct instr *instr;
	struct symbol *symbol;	/* NULL when no labels remain */

	symbol = first_defined_symbol();	/* NULL if no symbols */

	/* skip any symbols with NULL instruction pointers (start, PC == 0) */
	while (symbol && !symbol->after_instr)
		symbol = next_defined_symbol(symbol);

	/* walk the instruction list */
	list_for_each_entry(instr, &instructions, list) {
		// get instruction length, or best guess
		pc += instr_length(instr);
		//dump_instruction(instr);
		/* update value of any symbols referencing this instruction */
		while (symbol && symbol->after_instr == instr) {
			if (symbol->value != pc) {
				symbol->value = pc;
				symbols_changed++;
			}
			symbol = next_defined_symbol(symbol);
		}
	}

	/* all symbols should have been matched */
	if (ERR_ON(symbol)) {
		printf("dodgy symbol: ");
		dump_symbol(symbol);
	}

	printf("    symbols changed: %d\n", symbols_changed);
	return symbols_changed;
}

int main(void)
{
	struct instr *instr;

	yyparse();
	if (das_error) {
		printf("error, abort\n");
		return 1;
	}

	printf("After parse, before symbol resolution\n");
	dump_instructions();
	/* resolve instruction lengths and symbol values */
	do {
		printf(" + walk instruction list\n");
	} while (0 != walk_instructions());

	printf("After symbol resolution\n");
	dump_instructions();
	dump_symbols();
	binfile = fopen("out.bin", "w");
	if (!binfile) {
		fprintf(stderr, "opening output failed, %m");
	}
	list_for_each_entry(instr, &instructions, list) {
		writebin_instruction(instr);
	}
	fclose(binfile);
	return 0;
}
