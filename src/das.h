#ifndef DAS_H
#define DAS_H
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */

#define VERSION "0.15"
#define VERSTRING "das DCPU-16 Assembler, version " VERSION

extern struct options {
	int asm_print_pc;
	int asm_main_col;
	int asm_print_hex;
	int asm_hex_col;
	int asm_max_cols;
	int notch_style;
	int verbose;
	int big_endian;
} options;

#endif // DAS_H
