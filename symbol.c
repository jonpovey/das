/*
 * das things to do with symbols
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include "common.h"

static LIST_HEAD(defined_symbols);
static LIST_HEAD(undefined_symbols);

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

void dump_symbol(struct symbol *l)
{
	printf("%s = 0x%x\n", l->name, l->value);
}

void dump_symbols(void)
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
