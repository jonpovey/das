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
int symbol_value(struct symbol *sym);

/* Output */
void dump_symbol(struct symbol *l);
void dump_symbols(void);
int symbol_print_asm(char *buf, struct symbol *sym);

/* Cleanup */
void symbols_free(void);

#endif
