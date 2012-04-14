%{
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <stdio.h>

#include "das.h"

int yylex();

void yyerror(char *);
int get_lineno(void);
void parse_error(char *str);
%}

%union {
	int  integer;
	char *string;
	struct expr *expr;
	struct operand *operand;	// errrm
}

%token <string> SYMBOL LABEL
%token <integer> CONSTANT
%token <integer> GPREG
%token <integer> XREG
%token <integer> OP1 OP2

%type <expr> expr
%type <operand> operand
%type <integer> gpreg

%%

program:
	program line '\n'			{ /*printf("line\n");*/ }
	| program '\n'				/* empty line or comment */
	|
	;

line:
	instr
	| label
	| label instr
	;							

label:
	LABEL					{ label_parse($1); }
	;

instr:
	OP2 operand ',' operand		{ gen_instruction($1, $2, $4); /*printf("op2 ");*/ }
	| OP1 operand				{ gen_instruction($1, $2, NULL); /*printf("op1 ");*/ }
	| error						{ parse_error("bad instruction "); }
	;

operand:
	XREG						{ $$ = gen_operand($1, NULL, 0); }
	| gpreg						{ $$ = gen_operand($1, NULL, 0); }
	| expr						{ $$ = gen_operand(-1, $1, 0); }
	| '[' gpreg ']'				{ $$ = gen_operand($2, NULL, 1); }
	| '[' expr ']'				{ $$ = gen_operand(-1, $2, 1); }
	| '[' expr '+' gpreg ']'	{ $$ = gen_operand($4, $2, 1); }
	| '[' gpreg '+' expr ']'	{ $$ = gen_operand($2, $4, 1); }
	| error						{ parse_error("bad value "); }
	;

gpreg:
	GPREG						{ $$ = $1; }
	;

expr:
	CONSTANT					{ $$ = gen_const($1); }
	| SYMBOL					{ $$ = gen_symbol($1); }
	;

%%

void parse_error(char *str)
{
	// yylineno has advanced since the actual line this was found on
	fprintf(stderr, "line %d: parse error: %s\n", get_lineno() - 1, str);
	das_error = 1;
}
