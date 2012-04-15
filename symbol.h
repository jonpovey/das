#ifndef SYMBOL_H
#define SYMBOL_H
/*
 * das things to do with symbols
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include "instruction.h"
#include "list.h"

struct symbol;

/* Parse */
struct symbol* symbol_parse(char *name);
void label_parse(char *name);

/* Analysis */
struct symbol* first_defined_symbol(void);
struct symbol* next_defined_symbol(struct symbol *symbol);

/* Output */
void dump_symbol(struct symbol *l);
void dump_symbols(void);

/* Things that should not be exposed, hack hack */
struct symbol {
	char *name;
	int  value;
	struct instr *after_instr;	/* defined at PC after this instruction */
	struct list_head list;		/* on undefined or defined list */
};

#endif
