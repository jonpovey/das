/* das! */
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "das.h"
#include "dasdefs.h"
#include "expression.h"
#include "instruction.h"
#include "list.h"
#include "statement.h"
#include "symbol.h"

int yyparse(void);
int das_error = 0;

struct options options = {
	.asm_print_pc = 1,
//	.asm_print_pc = 0,
	.asm_main_col = 20,
	.asm_print_hex = 1,
	.asm_hex_col  = 60,
	.notch_style  = 1,
};

int main(void)
{
	int ret;
	u16 *binary = NULL;
	FILE* binfile;

	yyparse();
	if (das_error) {
		printf("error, abort\n");
		return 1;
	}

	/* resolve instruction lengths and symbol values */
	do {
		ret = statements_analyse();
		if (ret >= 0) {
			printf(" + Analysis pass: %d labels changed\n", ret);
		}
	} while (ret > 0);

	if (ret < 0) {
		fprintf(stderr, "Analysis error.\n");
		return 1;
	}

	printf("After symbol resolution\n");
	/* dump asm to stdout for now. later, to file by option switch */
	ret = statements_fprint_asm(stdout);
	if (ret < 0) {
		fprintf(stderr, "Dump error.\n");
		return 1;
	} else {
		printf("Dumped: %d lines\n", ret);
	}

	dump_symbols();

	binfile = fopen("out.bin", "w");
	if (!binfile) {
		fprintf(stderr, "opening output failed, %m");
	}
	ret = statements_get_binary(&binary);
	if (ret < 0) {
		fprintf(stderr, "Binary generation error.\n");
		return 1;
	}
	/* returned value is word count */
	fwrite(binary, sizeof(u16), ret, binfile);
	fclose(binfile);
	free(binary);
	statements_free();
	return 0;
}
