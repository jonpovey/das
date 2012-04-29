#include "common.h"

enum dat_types {
	DATTYPE_STRING,
	DATTYPE_EXPR,
};

struct dat_elem {
	int type;
	int nwords;
	union {
		struct expr *expr;
		unsigned char *data;	/* string */
	};
	struct dat_elem *next;
};

struct dat {
	/* maybe useful to store flags, precomputed binary and size here */
	struct dat_elem *first;
};

static struct statement_ops dat_statement_ops;

/*
 * Parse
 */
void gen_dat(struct dat_elem *elem)
{
	struct dat *dat = calloc(sizeof(*dat), 1);
	dat->first = elem;
	add_statement(dat, &dat_statement_ops);
}

struct dat_elem* dat_elem_follows(struct dat_elem *a, struct dat_elem *list)
{
	a->next = list;
	return a;
}

struct dat_elem* new_expr_dat_elem(struct expr *expr)
{
	struct dat_elem *e = calloc(sizeof(*e), 1);
	e->type = DATTYPE_EXPR;
	e->nwords = 1;
	e->expr = expr;
	/* next is null */
	return e;
}

struct dat_elem* new_string_dat_elem(char *str)
{
	struct dat_elem *e = calloc(sizeof(*e), 1);
	size_t len = strlen(str);

	printf("string dat: %s becomes:\n", str);
	str[len - 1] = 0;				/* chop off trailing quote */
	e->type = DATTYPE_STRING;
	e->data = malloc(strlen(str));
	/* e->data will be less one ", making space for NULL terminator */
	e->nwords = unescape_c_string(str + 1, e->data);
	printf("'%s'\n", (char*)e->data);
	/* e->next is null */
	return e;
}

/*
 * Analyse
 */

/*
 * Output
 */
static int dat_binary_size(void *private)
{
	int words = 0;
	struct dat *dat = private;
	struct dat_elem *e = dat->first;

	while (e) {
		words += e->nwords;
		e = e->next;
	}
	return words;
}

static int dat_get_binary(u16 *dest, void *private)
{
	int words = 0;
	struct dat *dat = private;
	struct dat_elem *e = dat->first;
	int i;

	while (e) {
		if (e->type == DATTYPE_STRING) {
			for (i = 0; i < e->nwords; i++) {
				*(dest + words) = e->data[i];
				words++;
			}
		} else {
			*(dest + words) = (u16)expr_value(e->expr);
			words++;
		}
		e = e->next;
	}
	return words;
}

static int dat_print_asm(char *buf, void *private)
{
	int count = 0;
	struct dat *dat = private;
	struct dat_elem *e = dat->first;

	count += sprintf(buf + count, "DAT ");
	while (e) {
		if (e->type == DATTYPE_STRING) {
			/* fixme for strings, they contain escapes.. */
			count += sprintf(buf + count, "\"");
			count += sprint_cstring(buf + count, e->data, e->nwords);
			count += sprintf(buf + count, "\"");
		} else {
			count += expr_print_asm(buf + count, e->expr);
		}
		e = e->next;
		if (e)
			count += sprintf(buf + count, ", ");
	}
	return count;
}

/*
 * Cleanup
 */
void dat_free_private(void *private)
{
	struct dat *dat = private;
	struct dat_elem *e, *next;

	e = dat->first;
	while (e) {
		next = e->next;
		if (e->type == DATTYPE_STRING) {
			free(e->data);
		} else {
			free_expr(e->expr);
		}
		free(e);
		e = next;
	}
	free(dat);
}

static struct statement_ops dat_statement_ops = {
	.analyse         = NULL,
	.get_binary_size = dat_binary_size,
	.get_binary      = dat_get_binary,
	.print_asm       = dat_print_asm,
	.free_private    = dat_free_private,
	.type            = STMT_DAT,
};
