/* das! */
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

int yyparse(void);
extern FILE *yyin;

#define HACK_ANALYSE_MAX		500
static int hack_loop_breaker = 0;

int das_error = 0;
int stdout_inuse = 0;
char *binpath;
char *asmpath;
char *dumppath;
char *dasname;

struct options options = {
	.asm_print_pc = 1,
//	.asm_print_pc = 0,
	.asm_main_col = 15,
	.asm_print_hex = 1,
	.asm_hex_col  = 60,
	.asm_max_cols = 80,
	.notch_style  = 1,
	.verbose = 0,
	.big_endian = 1,
};

void print_usage(void)
{
	fprintf(stderr, VERSTRING "\n");
	fprintf(stderr, "Latest and docs at: https://github.com/jonpovey/das\n\n");
	fprintf(stderr, "Usage: %s [OPTIONS] asmfile\n\n", dasname);
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o outfile         Write binary to outfile, default das-out.bin\n");
	fprintf(stderr, "  -v, --verbose      Be more chatty (normally silent on success)\n");
	fprintf(stderr, "  -d, --dump         Dump human-readable listing to stdout\n");
	fprintf(stderr, "  --dumpfile file    Dump to file instead\n");
	fprintf(stderr, "  --no-dump-pc       Omit PC column from dump; makes dump a valid source file\n");
	fprintf(stderr, "  --le               Generate little-endian binary (default big-endian)\n");
	fprintf(stderr, "\nThe character '-' for files means read/write to stdin/stdout instead.\n");
}

void suggest_help(void)
{
	fprintf(stderr, "For help run %s -h\n", dasname);
}

void handle_args(int argc, char **argv)
{
	int dump = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "o:vhd";
		static const struct option long_options[] = {
			{"dumpfile",	required_argument,	0, 0},
			{"le",			no_argument,		0, 0},
			{"no-dump-pc",	no_argument,		0, 0},
			{"dump",		no_argument,		0, 'd'},
			{"verbose",		no_argument,		0, 'v'},
			{},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);

		DBG("c:%d\n", c);
		if (c == -1) {
			break;
		}

		switch (c) {
		case 0:
			DBG("option_index:%d\n", option_index);
			switch (option_index) {
			case 0:
				dump = 1;
				dumppath = optarg;
				break;
			case 1:
				options.big_endian = 0;
				info("Big-endian binary mode selected\n");
				break;
			case 2:
				options.asm_print_pc = 0;
				break;
			default:
				BUG();
			}
			break;
		case 'd':
			dump = 1;
			break;
		case 'o':
			binpath = optarg;
			if (!strcmp("-", binpath))
				stdout_inuse++;
			break;
		case 'v':
			options.verbose = 1;
			stdout_inuse++;
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
			break;
		case '?':
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (dump && (!dumppath || !strcmp("-", dumppath))) {
		stdout_inuse++;
		dumppath = "-";		/* for later test, if using -d */
	}

	if (stdout_inuse > 1) {
		if (options.verbose)
			error("Can't mix verbose mode and file output to STDOUT");
		else
			error("Only one output file can use STDOUT");
		suggest_help();
		exit(EXIT_FAILURE);
	}

	if (optind < argc - 1) {
		error("Too many input files: Only one supported for now");
		DBG("optind:%d argc:%d\n", optind, argc);
		suggest_help();
		exit(EXIT_FAILURE);
	} else if (optind == argc) {
		print_usage();
		exit(EXIT_FAILURE);
	} else {
		/* exactly one argument: the input file path */
		asmpath = argv[optind];
		DBG("Input file: %s\n", asmpath);
	}

	if (!binpath) {
		/* guess binpath based on input filename */
		DBG("Guessing binpath\n");
		binpath = "das-out.bin";
	}

}

void reverse_words(u16 *bin, int nwords)
{
	while (nwords--) {
		/* mingw32 doesn't seem to have htons() so do it myself */
		*bin = *bin >> 8 | *bin << 8;
		bin++;
	}
}

int main(int argc, char **argv)
{
	int ret;
	int exitval = 0;
	u16 *binary = NULL;
	FILE *binfile, *asmfile, *dumpfile = 0;

	dasname = argv[0];

	handle_args(argc, argv);

	if (!strcmp("-", asmpath)) {
		asmfile = stdin;
		info("Input: stdin\n");
	} else {
		asmfile = fopen(asmpath, "r");
		info("Input file: %s\n", asmpath);
	}
	if (!asmfile) {
		error("Opening %s failed: %s", asmpath, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!strcmp("-", binpath)) {
		binfile = stdout;
		/* info pointless - verbose mode incompatible */
	} else {
		binfile = fopen(binpath, "w");
		info("Write binary to %s\n", binpath);
	}
	if (!binfile) {
		error("Writing %s failed: %s\n", binpath, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (dumppath) {
		if (!strcmp("-", dumppath)) {
			dumpfile = stdout;
			/* info pointless - verbose mode incompatible with stdout dump */
		} else {
			dumpfile = fopen(dumppath, "w");
			info("Dumping to %s\n", dumppath);
		}
		if (!dumpfile) {
			error("Dump to %s failed: %s\n", dumppath, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	yyin = asmfile;
	yyparse();
	if (das_error) {
		fprintf(stderr, "Aborting from parse error\n");
		return 1;
	}

	/* resolve instruction lengths and symbol values */
	do {
		ret = statements_analyse();
		if (ret >= 0) {
			info("Analysis pass: %d labels changed\n", ret);
			hack_loop_breaker++;
		}
	} while (ret > 0 && hack_loop_breaker < HACK_ANALYSE_MAX);

	if (hack_loop_breaker == HACK_ANALYSE_MAX && ret != 0) {
		fprintf(stderr, "Analysis still running after %d passes: "
				"Circular .equ reference?\n", hack_loop_breaker);
		return 1;
	}

	if (ret < 0) {
		fprintf(stderr, "Analysis error.\n");
		return 1;
	}

	if (dumpfile) {
		fprintf(dumpfile, "; Dump from " VERSTRING "\n");
		if (asmfile != stdin) {
			fprintf(dumpfile, "; Source file: %s\n", asmpath);
		}
		ret = statements_fprint_asm(dumpfile);
		if (ret < 0) {
			fprintf(stderr, "Dump error.\n");
			return 1;
		} else {
			info("Dumped: %d lines\n", ret);
		}
	}

	/* completely obsolete? */
//	dump_symbols();

	ret = statements_get_binary(&binary);
	if (ret < 0) {
		fprintf(stderr, "Binary generation error.\n");
		return 1;
	}
	/* returned value is word count. reverse words for big-endian storage */
	if (options.big_endian) {
		reverse_words(binary, ret);
	}
	if (ret != fwrite(binary, sizeof(u16), ret, binfile)) {
		fprintf(stderr, "Binary write error: %s\n", strerror(errno));
		exitval = 1;
	}
	fclose(binfile);
	free(binary);
	statements_free();
	symbols_free();
	return exitval;
}
