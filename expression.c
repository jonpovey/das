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

/* generate a expr for a const */
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
 * dump functions
 */
void dump_expr(struct expr *e)
{
	if (e->issymbol) {
		printf("%s(0x%x)", e->symbol->name, e->symbol->value);
	} else {
		printf("0x%x", e->value);
	}
}

/*
 * bit generation
 */
int expr_value(struct expr *expr)
{
	assert(expr);
	if (expr->issymbol) {
		return expr->symbol->value;
	} else {
		return expr->value;
	}
}

int expr_contains_symbol(struct expr *expr)
{
	//printf("contains symbol: %d\n", expr->issymbol);
	return expr->issymbol;
}
