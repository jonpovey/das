/*
 * das things to do with (integer) expressions
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 *
 */
#include "common.h"
#include "y.tab.h"

enum expr_type {
	EXPR_SYMBOL,
	EXPR_CONSTANT,
	EXPR_OPERATOR,
};

struct expr {
	int type;
	int maychange;
	int value;			/* valid shortcut if maychange = 0 */
	union {
		struct symbol *symbol;
		int op;
	};
	struct expr *left;	/* wasted space if not an operator node */
	struct expr *right;
};

int alloc_count, free_count;

/* internal unconditional (re)calculation */
static int expr_value_calc(struct expr *e)
{
	int right, left;

	assert(e);
	if (e->type == EXPR_SYMBOL) {
		return symbol_value(e->symbol);
	} else if (e->type == EXPR_CONSTANT) {
		return e->value;
	}
	/* operator */
	right = expr_value(e->right);
	switch (e->op) {
	case UMINUS: return - expr_value(e->right);
	case '~': return ~ expr_value(e->right);
	case '(': return expr_value(e->right);		/* (expr) is a no-op */
	}

	left = expr_value(e->left);
	switch (e->op) {
	case '-': return left - right;
	case '+': return left + right;
	case '*': return left * right;
	case '/':
		if (right == 0) {
			printf("divide by zero error in expression (return 0)\n");
			return 0;
		} else {
			return left / right;
		}
		break;
	case '|': return left | right;
	case '^': return left ^ right;
	case '&': return left & right;
	case LSHIFT: return left << right;
	case RSHIFT: return left >> right;
	default:
		fprintf(stderr, "BUG: unhandled operator %d\n", e->op);
	}
	return -1;
}

/*
 * Parse
 */
struct expr* gen_const(int val)
{
	//printf("gen_const: %d\n", val);
	struct expr *e = calloc(sizeof *e, 1);
	DBG_MEM("alloc %d: %p\n", ++alloc_count, e);
	e->type = EXPR_CONSTANT;
	e->maychange = 0;
	e->value = val;
	return e;
}

struct expr* gen_symbol_expr(struct symbol *sym)
{
	//printf("gen_symbol: %s\n", str);
	struct expr *e = calloc(sizeof *e, 1);
	DBG_MEM("alloc %d: %p\n", ++alloc_count, e);
	e->type = EXPR_SYMBOL;
	e->maychange = 1;
	e->value = 0;
	e->symbol = sym;
	symbol_mark_used(sym);
	return e;
}

struct expr* expr_op(int op, struct expr* left, struct expr* right)
{
	struct expr *e;

	assert(right);
	if (op != UMINUS && op != '~' && op != '(')
		assert(left);

	e = calloc(sizeof *e, 1);
	DBG_MEM("alloc %d: %p\n", ++alloc_count, e);
	e->type = EXPR_OPERATOR;
	e->op = op;
	e->left = left;
	e->right = right;

	if ((left && left->maychange) || (right && right->maychange)) {
		e->maychange = 1;
	} else {
		/* no symbols in subexpression, value can be known now */
		e->maychange = 0;
		e->value = expr_value_calc(e);	/* force calculation */
	}
	return e;
}

/*
 * Analysis
 */

int expr_maychange(struct expr *e)
{
	assert(e);
	return e->maychange;
}

int expr_value(struct expr *e)
{
	assert(e);
	if (e->maychange)
		return expr_value_calc(e);
	else
		return e->value;
}

/*
 * Output
 */

int expr_print_asm(char *buf, struct expr *e)
{
	int n = 0;

	/* could do resolved value printing with a toggle */
	if (e->type == EXPR_SYMBOL) {
		return symbol_print_asm(buf, e->symbol);
	} else if (e->type == EXPR_CONSTANT) {
		if (e->value < 0xf) {
			/* print small numbers as decimal without 0x */
			return sprintf(buf, "%d", e->value);
		} else {
			return sprintf(buf, "0x%x", e->value);
		}
	}

	/* else, operator */
	if (e->op == '(')
		n += sprintf(buf + n, "(");
	if (e->left)
		n += expr_print_asm(buf + n, e->left);
	switch (e->op) {
	case UMINUS: n += sprintf(buf + n, "-"); break;
	case '~':    n += sprintf(buf + n, "~"); break;
	case '+':
	case '-':
	case '*':
	case '/':
	case '&':
	case '^':
	case '|':
		n += sprintf(buf + n, " %c ", (char)e->op);
		break;
	case LSHIFT: n += sprintf(buf + n, " << "); break;
	case RSHIFT: n += sprintf(buf + n, " >> "); break;
	/* default nothing, parens */
	}
	if (e->right)
		n += expr_print_asm(buf + n, e->right);
	if (e->op == '(')
		n += sprintf(buf + n, ")");
	return n;
}

/* Cleanup */

void free_expr(struct expr *e)
{
	if (e->right) {
		DBG_MEM("free right: %p\n", e->right);
		free_expr(e->right);
	} else {
		DBG_MEM("nothing on right\n");
	}
	if (e->left) {
		DBG_MEM("free left: %p\n", e->left);
		free_expr(e->left);
	} else {
		DBG_MEM("nothing on left\n");
	}
	free(e);
	DBG_MEM("free %d: %p\n", ++free_count, e);
	/* symbols freed separately */
}
