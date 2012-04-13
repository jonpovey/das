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
	struct instr *def_after;	/* defined at PC after this instruction */
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
	int word_count;
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

/*
 * label list management
 */
static LIST_HEAD(defined_labels);	/* labels go on this list when :def found */
static LIST_HEAD(undefined_labels);	/* on this list if :def not found yet */

static struct label dummy = { .name = "dummy", .value = 0x100 };

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

/* encountered a label, not a labeldef. */
struct label* label_parse(char *name)
{
	// search label list (or hash, tree) for matching label
	// if it exists, return ptr
	// else create new label, add to store, return that
	return &dummy;
}

/* encountered a labeldef, not a label */
struct label* labeldef_parse(char *name)
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
		printf("def for undefined label '%s'\n", name); // dbg
		/* move to defined labels list */
		list_move_tail(&l->list, &defined_labels);
	}
	if (!l) {
		l = calloc(1, sizeof *l);
		l->name = strdup(name);
		list_add_tail(&l->list, &defined_labels);
	}
	/* associate with previous instruction parsed */
	printf("FIXME\n");
	// l->def_after = FIXME;
	return l;
}

/* test label (somehow) to see if value is resolved yet */
int label_resolved(struct label *l)
{
	assert(l);
	return l->value != -1;
}

/*
 * instruction list management
 */
static LIST_HEAD(instructions);

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
	v->word_count = 0;
	return v;
}

/* generate an instruction from an opcode and one or two values */
void gen_instruction(int opcode, struct value *vala, struct value *valb)
{
	struct instr*i = malloc(sizeof *i);
	i->opcode = opcode;
	i->vala = vala;
	i->valb = valb;
	//printf("add instruction to list\n");
	list_add_tail(&i->list, &instructions);
}

/*
 * dump functions
 */
void dump_num(struct num *n)
{
	if (n->islabel) {
		printf("%s", n->label->name);
		if (label_resolved(n->label))
			printf("(0x%x)", n->label->value);
		else
			printf("(?)");
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

void dump_instruction(struct instr*i)
{
	printf("%s ", op2str(i->opcode));
	dump_val(i->vala);
	if (i->valb) {
		printf(", ");
		dump_val(i->valb);
	}
	printf("\n");
}

/*
 * bit generation
 */
int num_getval(struct num *num)
{
	assert(num);
	if (num->islabel) {
		ERR_ON(!label_resolved(num->label));
		return num->label->value;
	} else {
		return num->value;
	}
}

/* validate completeness and generate bits for value */
void val_genbits(struct value *v)
{
	int numval;
	int words = 1;

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

	if (words == 2)
		v->nextbits = (u16)numval;
	v->word_count = words;
}

u16 val_firstbits(struct value *v)
{
	assert(v->word_count >= 1);
	//printf("firstbits: 0x%x\n", v->firstbits);
	return v->firstbits;
}

u16 val_nextbits(struct value *v)
{
	assert(v->word_count == 2);
	//printf("nextbits: 0x%x\n", v->nextbits);
	return v->nextbits;
}

/* write instruction binary to binfile */
void writebin_instruction(struct instr*i)
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
	if (i->vala->word_count == 2) {
		word = val_nextbits(i->vala);
		fwrite(&word, 2, 1, binfile);
	}
	if (i->valb && i->valb->word_count == 2) {
		word = val_nextbits(i->valb);
		fwrite(&word, 2, 1, binfile);
	}		
}

int main(void)
{
	struct instr*instr;

	yyparse();
	if (das_error) {
		printf("error, abort\n");
		return 1;
	}
	list_for_each_entry(instr, &instructions, list) {
		dump_instruction(instr);
	}
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
