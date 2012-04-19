/*
 * das things to do with assembly input statements
 *
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include "common.h"

#define MAX_BINARY_WORDS (1 << 16)			/* 16-bit (word) address space */
#define MAX_BINARY_BYTES (MAX_BINARY_WORDS * 2)

/* Implement polymorphism in C, Linux kernel-style */
struct statement {
	const struct statement_ops *ops;
	struct list_head list;		/* when on master input list */
	void *private;				/* pointer to type-specific data */
};

/* all parsed statements in order encountered */
static LIST_HEAD(statements);

statement* next_statement(statement *cur)
{
	if (cur->list.next == &statements)
		return NULL;
	else
		return list_entry(cur->list.next, statement, list);
}

/*
 * add a statement when encountered by parser. Call from e.g. instruction
 * and label parse handling / tree building code.
 */
void add_statement(void *private, const struct statement_ops *ops)
{
	statement *s = malloc(sizeof *s);
	s->ops = ops;
	s->private = private;
	list_add_tail(&s->list, &statements);
}

/*
 * Do one analysis pass of all statements; this used to be walk_instructions()
 * Compute statement size if known[1].
 * Maintain a running total of statement sizes (PC value).
 *
 * Labels are included in this statement list and will get their value set
 * in the analyse call.
 *
 * [1] Instruction size may not be known with literals that contain symbols,
 * if we aren't yet sure of symbol value. In this case assume we can use a short
 * literal until symbol is known.
 *
 * Call this function more than once.
 * On the first pass, symbols defined later will have unknown value.
 * On the second pass, symbols will have a possibly-correct value and some
 * instruction lengths may increase[2] due to literals becoming too long and
 * requiring the next-word literal form. 
 *
 * statements can only become longer. Keep running this function until no
 * symbol values change (function returns 0).
 *
 * In a pathological case (needing compile-time constant expressions and a
 * deliberate attempt at breakage, I think), this "bubbling-up" may have to
 * run many times, but not more times than there are instructions.
 *
 * [2] If later I implement compile-time constant expressions, a symbol could
 * change value such that a literal became smaller, e.g. (0x40 - foo) if
 * foo changes from 0x20 to 0x21, the literal would become 0x1f and no longer
 * require a next-word.
 * To guard against a potential infinite loop I intend to not permit
 * literals to become smaller. If they ever need a next-word, set a
 * "using next-word" flag for them and even if the value reduces, still use
 * next-word form.
 *
 * I should test that handling. Yes, I am paranoid.
 *
 * Return value: number of symbols whose value changed on this run
 */
int statements_analyse(void)
{
	statement *s;
	int pc = 0;
	int labels_changed = 0;
	int ret;

	if (list_empty(&statements)) {
		fprintf(stderr, "Error: No statements to work on\n");
		return -1;
	}

	list_for_each_entry(s, &statements, list) {
		/* some statements may have no analysis work (maybe DAT) */
		if (s->ops->analyse) {
			ret = s->ops->analyse(s->private, pc);
			if (ret < 0) {
				// analysis error!
				return ret;
			} else if (ret > 0) {
				labels_changed++;
			}
			// else done OK
		}
		/* some statements may have no binary size (e.g. labels) */
		if (s->ops->get_binary_size) {
			ret = s->ops->get_binary_size(s->private);
			if (ret < 0) {
				// eek
				return ret;
			} else {
				pc += ret;
			}
		}
	}
	// should be trace or maybe warn/error if > 64k
	printf("analysis end PC: 0x%x words\n", pc);
	return labels_changed;
}

/*
 * get binary representation of statements (assembler output).
 * *dest should be NULL or malloc'd buffer, will be realloc'd if not big enough.
 * (just setting it to 64K words should work.. right?)
 * return number of words or -1 if error
 */
int statements_get_binary(u16 **dest)
{
	statement *s;
	int ret, offset, size;

	*dest = realloc(*dest, MAX_BINARY_BYTES);
	offset = 0;

	list_for_each_entry(s, &statements, list) {
		if (s->ops->get_binary_size) {
			size = s->ops->get_binary_size(s->private);
			if (size < 0)
				return size;	// returned error

			if (offset + size > MAX_BINARY_WORDS) {
				fprintf(stderr, "Error: Binary size exceeds address space\n");
				return -1;
			}

			if (!s->ops->get_binary) {
				fprintf(stderr, "get_binary unsupported wtf?\n");
				return -1;
			}
			ret = s->ops->get_binary(*dest + offset, s->private);
			if (ret < 0)
				return ret;		// returned error

			offset += size;
		}
	}
	return offset;
}

/*
 * helper for statements_fprint_asm()
 * update *lines as we go. Don't print a newline at the end of the last line,
 * do_eol handler will do it.
 *
 * why do I sprintf() to a line buffer in the parent function but use fprintf()
 * here? Good question!
 */
void fprint_bin_chunk(FILE *f, u16 *binbuf, int binwords, int start_col,
						int pc, int *lines)
{
	int i;
	int col;

	for (i = 0; binwords; --binwords, i++, pc++) {
		if (0 == i % 8) {
			/* new line */
			fprintf(f, "\n");
			col = 0;
			if (options.asm_print_pc)
				col += fprintf(f, "%04x ", pc);

			/* pad to start column */
			col += fprintf(f, "%*c;", start_col - col, ' ');
		}
		fprintf(f, " %04x", binbuf[i]);
	}
}

/*
 * Write assembler representation of all statements to stream.
 * return number of lines written or -1 on error.
 * (0 lines might also be considered an input error)
 */
int statements_fprint_asm(FILE *f)
{
	char linebuf[1024];		/* should be big enough... */
	u16  binbuf[512];		/* ditto */
	int lines = 0, col = 0;
	int pc = 0;
	int asm_main_col = options.asm_main_col;
	statement *s;
	int i, ret;

	if (options.asm_print_pc) {
		asm_main_col += 5;
		// hex_col?
	}

	list_for_each_entry(s, &statements, list) {
		int do_eol = 1;
		int binwords = 0;

		if (col == 0) {
			/* start of a new line */
			if (options.asm_print_pc)
				col += sprintf(linebuf, "%04x ", pc);

			/* print labels with no extra indent, pad others */
			if (s->ops->type != STMT_LABEL)
				col += sprintf(linebuf + col, "%*c", asm_main_col - col, ' ');
		}

		/* print the statement */
		col += s->ops->print_asm(linebuf + col, s->private);

		/*
		 * pad instead of starting a new line IF:
		 * - this statement is a label
		 * - AND the next statatement is not a label
		 * - AND col is < asm_main_col (do EOL after very long labels)
		 */
		if (s->ops->type == STMT_LABEL && col < asm_main_col) {
			statement *next = next_statement(s);
			if (next && next->ops->type != STMT_LABEL)
				do_eol = 0;
		}

		/* if this statement has a size, get it */
		if (s->ops->get_binary_size) {
			binwords = s->ops->get_binary_size(s->private);
			if (binwords < 0)
				return binwords;	// error
			if (binwords > ARRAY_SIZE(binbuf)) {
				fprintf(stderr, "Can't handle giant statement, %d words!\n",
						binwords);
				return -1;
			}
		}

		/* if there is binary, annotate it (if in that mode) */
		if (binwords && options.asm_print_hex) {
			int pad = 1;
			if (col < options.asm_hex_col) {
				pad = options.asm_hex_col - col;
			}

			ret = s->ops->get_binary(binbuf, s->private);
			if (ret != binwords) {
				fprintf(stderr, "binwords mismatch!\n");
				return -1;
			}
			
			/* will the binary fit on this line? */
			if (col + pad + binwords * 5 + 2 > options.asm_max_cols) {
				/* nope. finish this line first then (no newline)*/
				fprintf(f, "%s", linebuf);
				lines++;
				col = 0;

				/* call helper to print chunk */
				fprint_bin_chunk(f, binbuf, binwords, asm_main_col + 4, pc,
								&lines);
			} else {
				/* fits on line */
				col += sprintf(linebuf + col, "%*c", pad, ' ');
				col += sprintf(linebuf + col, ";");
				for (i = 0; i < binwords; i++)
					col += sprintf(linebuf + col, " %04x", binbuf[i]);
			}
		}

		if (do_eol) {
			/* terminate and write line to stream */
			col += sprintf(linebuf + col, "\n");
			fprintf(f, "%s", linebuf);
			lines++;
			col = 0;
		} else {
			/* pad ready for next statement following label */
			col += sprintf(linebuf + col, "%*c", asm_main_col - col, ' ');
		}
		pc += binwords;
	}
	return lines;
}

/* free all statement storage and children */
void statements_free(void)
{
	statement *s, *temp;

	list_for_each_entry_safe(s, temp, &statements, list) {
		if (s->ops->free_private)
			s->ops->free_private(s->private);
		free(s);
	}
}
