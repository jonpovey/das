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
label storage
-------------

store name string and some kind of reference for figuring out the address it
resolves to. When resolved, store an int?

During parsing, encoutering a labeldef, associate that label with the preceding
statement (following statement not yet encountered). Instruction length not
known during parse time, or is it?

*/

// need forward decl for circular references
struct instr;

struct label {
	char *name;
	int  value;
	struct instr *after_instr;	/* defined at PC after this instruction */
	struct list_head list;		/* on undefined or defined list */
};

struct num {
	int islabel;
	union {
		int value;
		struct label *label;
	};
};

struct value {
	int indirect;
	int reg;
	struct num *num;
	int known_word_count;
	u16 firstbits;
	u16 nextbits;
};

struct instr {
	int opcode;
	struct value* vala;
	struct value* valb;
	int length_known;			/* 0 if unknown (maybe depends on labels) */
	struct list_head list;		/* for master list of instructions */
};

/* all parsed instructions in order encountered */
static LIST_HEAD(instructions);

/*
 * label list management
 */
static LIST_HEAD(defined_labels);	/* labels go on this list when :def found */
static LIST_HEAD(undefined_labels);	/* on this list if :def not found yet */

/* little helper function */
struct label* first_defined_label(void)
{
	if (list_empty(&defined_labels))
		return NULL;
	else
		return list_entry(defined_labels.next, struct label, list);
}

struct label* next_defined_label(struct label *label)
{
	if (label->list.next == &defined_labels)
		return NULL; /* no more labels */
	else
		return list_entry(label->list.next, struct label, list);
}

/* return label ptr if found (by name) in given list */
struct label* label_inlist(char *name, struct list_head *head)
{
	struct label *l;
	list_for_each_entry(l, head, list) {
		if (0 == strcmp(l->name, name))
			return l;
	}
	return NULL;
}

static void dump_label(struct label *l)
{
	printf("%s = 0x%x\n", l->name, l->value);
}

static void dump_labels(void)
{
	struct label *label;

	printf("defined labels:");
	if (list_empty(&defined_labels)) {
		printf(" None\n");
	} else {
		printf("\n");
		list_for_each_entry(label, &defined_labels, list)
			dump_label(label);
	}
	printf("undefined labels:");
	if (list_empty(&undefined_labels)) {
		printf(" None\n");
	} else {
		printf("\n");
		list_for_each_entry(label, &undefined_labels, list)
			dump_label(label);
	}
}

/* encountered a label, not a labeldef. */
struct label* label_parse(char *name)
{
	// search label list (or hash, tree) for matching label
	struct label *l;
	l = label_inlist(name, &defined_labels);
	if (!l)
		l = label_inlist(name, &undefined_labels);
	if (!l) {
		/* not seen a label with this name before */
		l = calloc(1, sizeof *l);
		l->name = strdup(name);
		list_add_tail(&l->list, &undefined_labels);
	}
	return l;
}

/*
 * encountered a labeldef, not a label.
 * called from parser, nothing to return, just note label def location
 */
void labeldef_parse(char *name)
{
	struct label *l;
	l = label_inlist(name, &defined_labels);
	if (l) {
		/* error, multiple definition */
		fprintf(stderr, "error: label '%s' redefinition\n", name);
		/* bubble it to latest definition.. bad, but consistent */
		list_move_tail(&l->list, &defined_labels);
	} else {
		l = label_inlist(name, &undefined_labels);
		if (l) {
			/* move to defined labels list */
			list_move_tail(&l->list, &defined_labels);
		}
	}
	if (!l) {
		l = calloc(1, sizeof *l);
		l->name = strdup(name);
		list_add_tail(&l->list, &defined_labels);
	}
	/*
	 * associate with latest instruction parsed.
	 * if no instructions yet, associate NULL. this is valid and means
	 * label value 0.
	 */
	if (list_empty(&instructions)) {
		//printf("associate labeldef %s with NULL\n", name);
		l->after_instr = NULL;
		l->value = 0;
	} else {
		l->after_instr = list_entry(instructions.prev, struct instr, list);
		//printf("associate labeldef %s with instruction %p\n", name,
		//		l->after_instr);
	}
}

/* generate a num for a const */
struct num* gen_const(int val)
{
	//printf("gen_const: %d\n", val);
	struct num *n = malloc(sizeof *n);
	n->islabel = 0;
	n->value = val;
	return n;
}

struct num* gen_label(char *str)
{
	//printf("gen_label: %s\n", str);
	struct num *n = malloc(sizeof *n);
	n->islabel = 1;
	n->label = label_parse(str);
	return n;
}

/* generate a value from reg (maybe -1), num (maybe null), indirect flag */
struct value* gen_value(int reg, struct num *num, int indirect)
{
	//printf("gen_value(reg:%d num:%p %sdirect)\n", reg, num, indirect ? "in":"");
	struct value *v = calloc(1, sizeof *v);
	v->indirect = indirect;
	v->reg = reg;
	v->num = num;
	v->known_word_count = 0;
	return v;
}

/* generate an instruction from an opcode and one or two values */
void gen_instruction(int opcode, struct value *vala, struct value *valb)
{
	struct instr* i = malloc(sizeof *i);
	i->opcode = opcode;
	i->vala = vala;
	i->valb = valb;
	//printf("add instruction %p to list\n", i);
	list_add_tail(&i->list, &instructions);
}

/*
 * dump functions
 */
void dump_num(struct num *n)
{
	if (n->islabel) {
		printf("%s(0x%x)", n->label->name, n->label->value);
	} else {
		printf("0x%x", n->value);
	}
}

void dump_val(struct value *v)
{
	assert(v);
	if (v->indirect)
		printf("[");
	if (v->num)
		dump_num(v->num);
	if (v->num && v->reg != -1)
		printf(" + ");
	if (v->reg != -1)
		printf("%s", reg2str(v->reg));
	if (v->indirect)
		printf("]");
}

void dump_instruction(struct instr *i)
{
	printf("%s ", op2str(i->opcode));
	dump_val(i->vala);
	if (i->valb) {
		printf(", ");
		dump_val(i->valb);
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
int num_getval(struct num *num)
{
	assert(num);
	if (num->islabel) {
		return num->label->value;
	} else {
		return num->value;
	}
}

int num_contains_label(struct num *num)
{
	return num->islabel;
}

/* (calculate and) get the word count needed to represent a value */
int value_num_words(struct value *val)
{
	int words;
	int maychange = 0;

	/* already calculated, and is a fixed size? */
	if (val->known_word_count > 0)
		return val->known_word_count;

	if (val->num) {
		if (val->indirect) {
			/* all indirect forms use next-word */
			words = 2;
		} else {
			/* literal, is it small? */
			if (num_getval(val->num) < 0x20) {
				/* small.. at least for now */
				words = 1;
				maychange = num_contains_label(val->num);
			} else {
				words = 2;
			}
		}
	} else {
		/* no num, next-word never used */
		words = 1;
	}

	/*
	 * instructions are never allowed to become smaller, to ward off
	 * infinite loops. If words == 2, maychange is not set.
	 */
	if (!maychange) {
		val->known_word_count = words;
	} else {
		//printf("DBG: this value length may change!\n");
	}

	return words;
}

int value_needs_nextword(struct value *val)
{
	return value_num_words(val) - 1;
}

/* validate completeness and generate bits for value */
void val_genbits(struct value *v)
{
	int numval;
	int words = 1;	/* correctness check */

	// FIXME test this actually works
	if (0 == v->known_word_count) {
		/* label resolution has finished, size really is fixed now */
		v->known_word_count = value_num_words(v);
	}

	if (v->num)
		numval = num_getval(v->num);

	if (v->reg != -1)
		v->firstbits = reg2bits(v->reg);
	else
		v->firstbits = 0;

	if (v->firstbits & 0x10) {			// magic numbers ahoy!
		/* x-reg, bits set already */
		ERR_ON(v->num);
	} else if (v->reg < 0) {
		/* no reg. have a num. */
		if (v->indirect || numval > 0x1f) {
			if (v->indirect) {
				/* [next word] form */
				v->firstbits = 0x1e;
			} else {
				/* next word literal */
				v->firstbits = 0x1f;
			}
			v->nextbits = (u16)numval;
			words = 2;
		} else {
			/* small literal */
			v->firstbits = 0x20 | (u16)numval;
		}
	} else {
		/* gpreg in some form */
		if (v->indirect) {
			if (v->num) {
				/* [next word + register] */
				v->firstbits |= 0x10;
				words = 2;
			} else {
				/* [gp register] */
				v->firstbits |= 0x08;
			}
		}
		/* else plain gpreg, nothing to modify */
	}

	ERR_ON(words != v->known_word_count); // if true, assembler bug

	if (words == 2)
		v->nextbits = (u16)numval;
}

u16 val_firstbits(struct value *v)
{
	assert(v->known_word_count >= 1);
	//printf("firstbits: 0x%x\n", v->firstbits);
	return v->firstbits;
}

u16 val_nextbits(struct value *v)
{
	assert(v->known_word_count == 2);
	//printf("nextbits: 0x%x\n", v->nextbits);
	return v->nextbits;
}

/* return true if value is fixed length */
int value_fixed_len(struct value *val)
{
	if (val->known_word_count > 0)
		return 1;

	/* only false if label-based literal. */
	if (!val->indirect && val->num) {
		/* literal */
		return num_contains_label(val->num);
	}
	return 0;
}

/*
 * get instruction length. final length may increase due to label value
 * changes (but guaranteed length will never decrease)
 */
int instr_length(struct instr *i) {
	int len;

	/* shortcut if entirely static */
	if (i->length_known > 0)
		return i->length_known;

	len = 1 + value_needs_nextword(i->vala);
	if (i->valb)
		len += value_needs_nextword(i->valb);

	if (value_fixed_len(i->vala) && (!i->valb || value_fixed_len(i->valb))) {
		/* not going to change, can shortcut next time */
		i->length_known = len;
	}
	return len;
}

/* write instruction binary to binfile */
void writebin_instruction(struct instr *i)
{
	u16 word = 0;

	assert(i->vala);
	val_genbits(i->vala);
	if (i->valb)
		val_genbits(i->valb);

	if (i->opcode == 0) {
		/* special case JSR */
		word |= 1 << 4;
		word |= val_firstbits(i->vala) << 10;
	} else {
		assert(i->valb);
		assert(i->opcode > 0 && i->opcode < 16);
		word |= i->opcode;
		word |= val_firstbits(i->vala) << 4;
		word |= val_firstbits(i->valb) << 10;
	}
	fwrite(&word, 2, 1, binfile);
	if (value_needs_nextword(i->vala)) {
		word = val_nextbits(i->vala);
		fwrite(&word, 2, 1, binfile);
	}
	if (i->valb && value_needs_nextword(i->valb)) {
		word = val_nextbits(i->valb);
		fwrite(&word, 2, 1, binfile);
	}
}

/*
 * Walk the list of instructions.
 * Compute instruction size if known[1].
 * Maintain a running total of instruction sizes (PC value).
 *
 * Simultaneously walk the list of labeldefs, which is also in memory order.
 * When a label matches the current instruction, set that label's value to
 * the current PC.
 *
 * [1] Instruction size may not be known with literals that contain labels,
 * if we aren't yet sure of label value. In this case assume we can use a short
 * literal until label is known.
 *
 * Call this function more than once.
 * On the first pass, labels defined later will have unknown value.
 * On the second pass, labels will have a possibly-correct value and some
 * instruction lengths may increase[2] due to literals becoming too long and
 * requiring the next-word literal form. 
 *
 * Instructions can only become longer. Keep running this function until no
 * label values change (function returns 0).
 *
 * In a pathological case (needing compile-time constant expressions and a
 * deliberate attempt at breakage, I think), this "bubbling-up" may have to
 * run many times, but not more times than there are instructions.
 *
 * [2] If later I implement compile-time constant expressions, a label could
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
 * Return value: number of labels whose value changed on this run
 */
int walk_instructions(void)
{
	int labels_changed = 0;
	int pc = 0;
	struct instr *instr;
	struct label *label;	/* NULL when no labeldefs remain */

	label = first_defined_label();	/* NULL if no labels */

	/* skip any labels with NULL instruction pointers (start, PC == 0) */
	while (label && !label->after_instr)
		label = next_defined_label(label);

	/* walk the instruction list */
	list_for_each_entry(instr, &instructions, list) {
		// get instruction length, or best guess
		pc += instr_length(instr);

		/* update value of any labels referencing this instruction */
		while (label && label->after_instr == instr) {
			if (label->value != pc) {
				label->value = pc;
				labels_changed++;
			}
			label = next_defined_label(label);
		}
	}

	/* all labels should have been matched */
	if (ERR_ON(label)) {
		printf("dodgy label: ");
		dump_label(label);
	}

	return labels_changed;
}

int main(void)
{
	struct instr *instr;

	yyparse();
	if (das_error) {
		printf("error, abort\n");
		return 1;
	}

	printf("After parse, before label resolution\n");
	dump_instructions();
	/* resolve instruction lengths and label values */
	do {
		printf(" + walk instruction list\n");
	} while (0 != walk_instructions());

	printf("After label resolution\n");
	dump_instructions();
	dump_labels();
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
