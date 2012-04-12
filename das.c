/* das! */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "das.h"
#include "dasdefs.h"
#include "list.h"

int yyparse(void);
int das_error = 0;
FILE* binfile;

/*
instruction value representation
--------------------------------

numbers maybe labels which have to be resolved to numeric values later.
labels should be stored as a pointer to a label struct (in a list of labels)

numbers can also be constants which are easy

so a number needs to have a type (const or label) and a value (int or ptr)

maybe simplest to represent all values as having:
	a register: xreg, gpreg, or null
	a number: const int (maybe 0) or label
	a flag to say if the access is [indirect]
	a flag to say register not used (or could be sentinel register value)

register value could be as for machine code values:
	0-7       gpreg
	0x18-0x1d xreg
	-1        "no register"

instruction length of values
----------------------------

Any with "next word" are +1 word long, others fit in first word.
"next word" types are:
	[reg + any num]
	[any num]
	any num > 0x1f

labels as literals could be under 0x1f but that could be indeterminate as
instruction length depends on label value, but label value depends on
instruction length. So let's say any label is forced to a "next word" literal.

Rules for deciding value length:

val				reg	num	indirect	len	ok
gpreg			gp	0	0			0	y
xreg			x	0	0			0	y
[gpreg]			gp	0	1			0	y
[gpreg + num]	gp	!0	1			1
[num]			0	*	1			1	y
big literal		0	big	0			1
small literal	0	sm	0			0	y

if (reg == null) {
	if (indirect == 0 && num.type == const && num.val <= 0x1f)
		valsize = 0;	// small literal
	else
		valsize = 1;	// big literal or [num]
} else {
	if (num.type == label || num.value != 0)
		valsize = 1;	// [gpreg + num]
	else
		valsize = 0;	// gpreg, xreg, or [gpreg]
}

label storage
-------------

store name string and some kind of reference for figuring out the address it
resolves to. When resolved, store an int?

During parsing, encoutering a labeldef, associate that label with the preceding
statement (following statement not yet encountered). Instruction length not
known during parse time, or is it?

*/

struct label {
	char *name;
	int  value;
	// line number, or associated statement?
	struct list_head list;
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
};

struct instruction {
	int opcode;
	struct value* val1;
	struct value* val2;
	struct list_head list;
};

/* label list management */

static struct label dummy = { .name = "dummy", .value = 0x100 };

struct label* label_exists(char *str)
{
	// search label list (or hash, tree) for matching label
	// if it exists, return ptr
	// else create new label, add to store, return that
	return &dummy;
}

/* test label (somehow) to see if value is resolved yet */
int label_resolved(struct label *l)
{
	assert(l);
	return l->value != -1;
}

/* instruction list management */
static LIST_HEAD(instructions);

/* generate a num for a const */
struct num* gen_const(int val)
{
	printf("gen_const: %d\n", val);
	struct num *n = malloc(sizeof *n);
	n->islabel = 0;
	n->value = val;
	return n;
}

struct num* gen_label(char *str)
{
	printf("gen_label: %s\n", str);
	struct num *n = malloc(sizeof *n);
	n->islabel = 1;
	n->label = label_exists(str);
	return n;
}

/* generate a value from reg (maybe -1), num (maybe null), indirect flag */
struct value* gen_value(int reg, struct num *num, int indirect)
{
	printf("gen_value(reg:%d num:%p %sdirect)\n", reg, num, indirect ? "in":"");
	struct value *v = malloc(sizeof *v);
	v->indirect = indirect;
	v->reg = reg;
	v->num = num;
	return v;
}

/* generate an instruction from an opcode and one or two values */
void gen_instruction(int opcode, struct value *val1, struct value *val2)
{
	struct instruction *i = malloc(sizeof *i);
	i->opcode = opcode;
	i->val1 = val1;
	i->val2 = val2;
	printf("add instruction to list\n");
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

void dump_instruction(struct instruction *i)
{
	printf("%s ", op2str(i->opcode));
	dump_val(i->val1);
	if (i->val2) {
		printf(", ");
		dump_val(i->val2);
	}
	printf("\n");
}

/* write instruction binary to binfile */
void writebin_instruction(struct instruction *i)
{
	u16 firstword = 0;
	fwrite(&firstword, 2, 1, binfile);
}

int main(void)
{
	struct instruction *instr;

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
