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
#include "expression.h"
#include "instruction.h"
#include "list.h"
#include "symbol.h"

int yyparse(void);
int das_error = 0;
FILE* binfile;

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
		instruction_fwritebin(binfile, instr);
	}
	fclose(binfile);
	return 0;
}
