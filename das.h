#ifndef DAS_H
#define DAS_H
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
extern int das_error;

extern struct options {
	int asm_print_pc;
	int asm_main_col;
	int asm_print_hex;
	int asm_hex_col;
	int notch_style;
} options;

#endif // DAS_H
