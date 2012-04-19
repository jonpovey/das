/*
 * das things to do with (integer) expressions
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 *
 * Simple for now, todo later: more complex compile-time expressions
 */
#include "common.h"

struct expr {
	int issymbol;
	union {
		int value;
		struct symbol *symbol;
	};
};

/*
 * Parse
 */
struct expr* gen_const(int val)
{
	//printf("gen_const: %d\n", val);
	struct expr *e = malloc(sizeof *e);
	e->issymbol = 0;
	e->value = val;
	return e;
}

struct expr* gen_symbol(char *str)
{
	//printf("gen_symbol: %s\n", str);
	struct expr *e = malloc(sizeof *e);
	e->issymbol = 1;
	e->symbol = symbol_parse(str);
	return e;
}

/*
 * Analysis
 */

int expr_contains_symbol(struct expr *expr)
{
	//printf("contains symbol: %d\n", expr->issymbol);
	return expr->issymbol;
}

int expr_value(struct expr *expr)
{
	assert(expr);
	if (expr->issymbol) {
		return expr->symbol->value;
	} else {
		return expr->value;
	}
}

/*
 * Output
 */

int expr_print_asm(char *buf, struct expr *e)
{
	if (e->issymbol) {
		return symbol_print_asm(buf, e->symbol);
	} else {
		/* warn or something if truncating to 16-bit? */
		if (e->value < 0xf) {
			/* print small numbers as decimal without 0x */
			return sprintf(buf, "%d", e->value);
		} else {
			return sprintf(buf, "0x%x", e->value);
		}
	}
}

/* Cleanup */

void free_expr(struct expr *e)
{
	free(e);
	/* symbols freed separately */
}
