#ifndef EXPRESSION_H
#define EXPRESSION_H
/*
 * das things to do with (integer) expressions
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */

struct expr;
/* FIXME function names are fairly yuck */

/* Parse */
struct expr* gen_const(int val);
struct expr* gen_symbol(char *str);
struct expr* expr_op(int op, struct expr* left, struct expr* right);

/* Analyse */
int expr_value(struct expr *expr);
int expr_maychange(struct expr *expr);

/* Output */
void dump_expr(struct expr *e);
int expr_print_asm(char *buf, struct expr *e);

/* Cleanup */
void free_expr(struct expr *e);

#endif
