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

struct symbol {
	char *name;
	int  flags;
	int  value;
	struct expr *expr;			/* exists if this is a .set symbol */
	struct list_head list;		/* on all_symbols list */
};

static LIST_HEAD(all_symbols);
static const struct statement_ops label_statement_ops;
static const struct statement_ops equ_statement_ops;

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

void check_redefine(struct symbol *s)
{
	if (s->flags & SYM_LABEL || s->flags & SYM_DEF) {
		error("Error: symbol '%s' redefined", s->name);
	}
	return;
}

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
	check_redefine(s);
	s->flags |= SYM_LABEL;
	add_statement(s, &label_statement_ops);
}

void directive_equ(struct symbol *s, struct expr *e)
{
	/* FIXME: detect and error on circular references */
	check_redefine(s);
	s->flags |= SYM_DEF;
	s->expr = e;
	add_statement(s, &equ_statement_ops);
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
		TRACE1("label %s changed: %d -> %d\n", sym->name, sym->value, pc);
		sym->value = pc;
		return 1;
	}
	return 0;
}

/*
 * When an .equ directive is analysed, update symbol value from expression.
 * If changed, return 1
 */
static int equ_analyse(void *private, int pc)
{
	struct symbol *s = private;
	int value;
	assert(s->expr);
	assert(s->flags & SYM_DEF);
	value = expr_value(s->expr);
	if (s->value != value) {
		TRACE1("symbol %s changed: %d -> %d\n", s->name, s->value, pc);
		s->value = value;
		return 1;
	}
	return 0;
}

int symbol_value(struct symbol *sym)
{
	return sym->value;
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

static int equ_print_asm(char *buf, void *private)
{
	int count;
	struct symbol *s = private;

	assert(s->expr);
	assert(s->flags & SYM_DEF);

	count = sprintf(buf, ".equ %s, ", s->name);
	count += expr_print_asm(buf + count, s->expr);
	return count;
}

/* Cleanup */
void symbols_free(void)
{
	struct symbol *sym, *temp;

	list_for_each_entry_safe(sym, temp, &all_symbols, list) {
		assert(sym->name);
		free(sym->name);
		free(sym);
	}
	INIT_LIST_HEAD(&all_symbols);
}

static const struct statement_ops label_statement_ops = {
	.analyse         = label_analyse,
	.get_binary_size = NULL,	/* labels have no binary output */
	.print_asm       = label_print_asm,
	.free_private    = NULL,	/* symbols will be freed separately */
	.type            = STMT_LABEL,
};

static const struct statement_ops equ_statement_ops = {
	.analyse         = equ_analyse,
	.get_binary_size = NULL,	/* equ directives have no binary output */
	.print_asm       = equ_print_asm,
	.free_private    = NULL,	/* symbols will be freed separately */
	.type            = STMT_DIRECTIVE,
};
