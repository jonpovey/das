#ifndef STATEMENT_H
#define STATEMENT_H
/*
 * das things to do with assembly input statements
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <stdio.h>

#include "dasdefs.h"

enum stmt_type {
	STMT_NONE,
	STMT_INSTRUCTION,
	STMT_LABEL,
	STMT_DAT,
	STMT_DIRECTIVE,
};

/* hide struct implementation detail inside statement.c. Modular! */
typedef struct statement statement;

/* types of statements implement some or all of these method callbacks */
struct statement_ops {
	int (*validate)(void *private);        /* see statements_validate() */
	int (*analyse)(void *private, int pc); /* see statements_analyse() */
	int (*freeze)(void *private);          /* see statements_freeze() */

	/* get_binary_size() may not be implemented for e.g. labels */
	int (*get_binary_size)(void *private);

	/*
	 * get binary into dest, return number of words or -1 for error?
	 * such as "don't know yet" maybe.
	 * Assumes getting the binary is valid at this stage of processing.
	 */
	int (*get_binary)(u16 *dest, void *private);

	/*
	 * print_asm(): print this statement to dest as assembly.
	 * return length of string
	 * (possible buffer overrun problems here.)
	 */
	int (*print_asm)(char *dest, void *private);

	/* free child data during cleanup */
	void (*free_private)(void *private);
	
	enum stmt_type type;	/* not an "operation", but.. */
};

void add_statement(void *private, const struct statement_ops *ops);
int statements_validate(void);
int statements_analyse(void);
int statements_freeze(void);
int statements_get_binary(u16 **dest);
int statements_fprint_asm(FILE *f);
void statements_free(void);

#endif
