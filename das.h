#ifndef DAS_H
#define DAS_H
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */

#define VERSION "0.9"
#define VERSTRING "BlueDAS DCPU-16 Assembler, version " VERSION

extern int das_error;

extern struct options {
	int asm_print_pc;
	int asm_main_col;
	int asm_print_hex;
	int asm_hex_col;
	int asm_max_cols;
	int notch_style;
	int verbose;
} options;

/* fixme: better errors and warnings, with line numbers and such */
#define error(fmt, args...) do { \
	fprintf(stderr, "error: " fmt "\n", ##args); \
	das_error = 1; \
} while (0)

#define warn(fmt, args...) do { \
	fprintf(stderr, "warning: " fmt "\n", ##args); \
} while (0)

#endif // DAS_H
