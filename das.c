/* das! */

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
	struct num num;
};
