/* das! */
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

int yyparse(void);
extern FILE *yyin;

int das_error = 0;
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
};

void print_usage(void)
{
	fprintf(stderr, VERSTRING "\n");
	fprintf(stderr, "Latest and docs at: https://github.com/jonpovey/das\n\n");
	fprintf(stderr, "Usage: %s [OPTIONS] asmfile\n\n", dasname);
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -o outfile         Write binary to outfile, default das-out.bin\n");
	fprintf(stderr, "  -v, -verbose       Be more chatty (normally silent on success)\n");
	fprintf(stderr, "  -d, -dump          Dump human-readable listing, default to das-dump.txt\n");
	fprintf(stderr, "  -dumpto file       Specify the dump filename (implies -d)\n");
	fprintf(stderr, "\nThe character '-' for asmfile or outfile means read/write to stdin/stdout.\n");
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
			{"dump",		no_argument,		0, 'd'},
			{"verbose",		no_argument,		0, 'v'},
			{},
		};

		int c = getopt_long_only(argc, argv, short_options,
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
				DBG("dumppath:%s\n", dumppath);
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
			break;
		case 'v':
			options.verbose = 1;
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

	if (optind < argc - 1) {
		error("Too many input files: Only one supported for now\n");
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

	if (dump && !dumppath) {
		DBG("Guess dump path\n");
		dumppath = "das-dump.txt";
	}
}

void reverse_words(u16 *bin, int nwords)
{
	while (nwords--) {
		*bin = htons(*bin);
		bin++;
	}
}		

int main(int argc, char **argv)
{
	int ret;
	u16 *binary = NULL;
	FILE *binfile, *asmfile, *dumpfile = 0;

	dasname = argv[0];

	handle_args(argc, argv);

	if (!strcmp("-", asmpath)) {
		asmfile = stdin;
	} else {
		asmfile = fopen(asmpath, "r");
	}
	if (!asmfile) {
		fprintf(stderr, "Opening %s failed: %m\n", asmpath);
		exit(EXIT_FAILURE);
	}

	if (!strcmp("-", binpath)) {
		if (options.verbose) {
			error("Can't mix verbose output and binary to STDOUT");
			exit(EXIT_FAILURE);
		}
		binfile = stdout;
	} else {
		binfile = fopen(binpath, "w");
	}
	if (!binfile) {
		fprintf(stderr, "Writing %s failed: %m\n", binpath);
		exit(EXIT_FAILURE);
	}

	if (dumppath) {
		dumpfile = fopen(dumppath, "w");
		if (!dumpfile) {
			fprintf(stderr, "Dump to %s failed: %m\n", dumppath);
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
		}
	} while (ret > 0);

	if (ret < 0) {
		fprintf(stderr, "Analysis error.\n");
		return 1;
	}

	if (dumpfile) {
		fprintf(dumpfile, "; Dump from " VERSTRING "\n");
		if (asmfile != stdin) {
			fprintf(dumpfile, "; Source file: %s\n", asmpath);
		}
		/* dump asm to stdout for now. later, to file by option switch */
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
	/*
	 * returned value is word count. reverse words for big-endian storage
	 * (seems to be the de-facto spec)
	 */
	reverse_words(binary, ret);
	fwrite(binary, sizeof(u16), ret, binfile);
	fclose(binfile);
	free(binary);
	statements_free();
	symbols_free();
	return 0;
}
