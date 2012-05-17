#ifndef EXPRESSION_H
#define EXPRESSION_H
/*
 * das things to do with (integer) expressions
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */

struct expr;

#include "symbol.h"
#include "output.h"

/* Parse */
struct expr* gen_const_expr(LOCTYPE loc, int val);
struct expr* gen_symbol_expr(LOCTYPE loc, struct symbol *sym);
struct expr* gen_op_expr(LOCTYPE loc, int op, struct expr* left,
						struct expr* right);

/* Analyse */
void expr_validate(struct expr *e);
void expr_freeze(struct expr *e);
int expr_value(struct expr *expr);
int expr_maychange(struct expr *expr);

/* Output */
void dump_expr(struct expr *e);
int expr_print_asm(char *buf, struct expr *e);

/* Cleanup */
void free_expr(struct expr *e);

#endif
