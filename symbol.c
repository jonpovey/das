/*
 * das things to do with symbols
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include "common.h"

enum sym_flags {
	SYM_LABEL = 0x1,	/* definition label found */
	SYM_USED  = 0x2,	/* label applied somewhere (expression).. ? */
	SYM_DEF   = 0x4,	/* explicitly defined (not label) .. ?*/
};

static LIST_HEAD(all_symbols);
static const struct statement_ops label_statement_ops;

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

/*
 * Parse
 */

struct symbol* symbol_parse(char *name)
{
	// search symbol list (or hash, tree) for matching symbol
	struct symbol *sym;
	sym = symbol_inlist(name, &all_symbols);
	if (!sym) {
		/* not seen a symbol with this name before */
		sym = calloc(1, sizeof *sym);
		sym->name = strdup(name);
		list_add_tail(&sym->list, &all_symbols);
	}
	return sym;
}

void label_parse(char *name)
{
	struct symbol *s;
	s = symbol_parse(name);
	if (s->flags & SYM_LABEL) {
		fprintf(stderr, "Error: label '%s' redefined\n", name);
		// blow up
	}
	s->flags |= SYM_LABEL;
	add_statement(s, &label_statement_ops);
}

/*
 * Analysis
 */

/*
 * when a label is called in analysis pass, set its value to PC.
 * if value changed, return 1
 */
static int label_analyse(void *private, int pc)
{
	struct symbol *sym = private;
	if (sym->value != pc) {
		sym->value = pc;
		return 1;
	}
	return 0;
}

/*
 * Output
 */

void dump_symbol(struct symbol *sym)
{
	/* FIXME: print info about if defined yet, etc */
	if (sym->flags & SYM_LABEL)
		printf("%s = 0x%x\n", sym->name, sym->value);
	else
		printf("%s UNDEFINED\n", sym->name);
}

void dump_symbols(void)
{
	struct symbol *symbol;

	printf("symbols:");
	if (list_empty(&all_symbols)) {
		printf(" None\n");
	} else {
		printf("\n");
		list_for_each_entry(symbol, &all_symbols, list)
			dump_symbol(symbol);
	}
}

int symbol_print_asm(char *buf, struct symbol *sym)
{
	return sprintf(buf, "%s", sym->name);
}

static int label_print_asm(char *buf, void *private)
{
	struct symbol *sym = private;
	const char *fmt;

	if (options.notch_style)
		fmt = ":%s";
	else
		fmt = "%s:";

	return sprintf(buf, fmt, sym->name);
}

static const struct statement_ops label_statement_ops = {
	.analyse         = label_analyse,
	.get_binary_size = NULL,	/* labels have no binary output */
	.print_asm       = label_print_asm,
	.free_private    = NULL,	/* symbols will be freed separately */
	.type            = STMT_LABEL,
};
