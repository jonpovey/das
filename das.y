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
	int  intval;
	char *stringval;
	struct num *numval;
	struct value *valval;	// errrm
}

%token <stringval> LABEL LABELDEF
%token <intval> CONSTANT
%token <intval> GPREG
%token <intval> XREG
%token <intval> OP1 OP2

%type <numval> num
%type <valval> value
%type <intval> gpreg

%%

program:
	program line '\n'			{ /*printf("line\n");*/ }
	| program '\n'				/* empty line or comment */
	|
	;

line:
	instr
	| labeldef
	| labeldef instr
	;							

labeldef:
	LABELDEF					{ labeldef_parse($1); }
	;

instr:
	OP2 value ',' value			{ gen_instruction($1, $2, $4); /*printf("op2 ");*/ }
	| OP1 value					{ gen_instruction($1, $2, NULL); /*printf("op1 ");*/ }
	| error						{ parse_error("bad instruction "); }
	;

value:
	XREG						{ $$ = gen_value($1, NULL, 0); }
	| gpreg						{ $$ = gen_value($1, NULL, 0); }
	| num						{ $$ = gen_value(-1, $1, 0); }
	| '[' gpreg ']'				{ $$ = gen_value($2, NULL, 1); }
	| '[' num ']'				{ $$ = gen_value(-1, $2, 1); }
	| '[' num '+' gpreg ']'		{ $$ = gen_value($4, $2, 1); }
	| '[' gpreg '+' num ']'		{ $$ = gen_value($2, $4, 1); }
	| error						{ parse_error("bad value "); }
	;

gpreg:
	GPREG						{ $$ = $1; }
	;

num:
	CONSTANT					{ $$ = gen_const($1); }
	| LABEL						{ $$ = gen_label($1); }
	;

%%

void parse_error(char *str)
{
	// yylineno has advanced since the actual line this was found on
	fprintf(stderr, "line %d: parse error: %s\n", get_lineno() - 1, str);
	das_error = 1;
}
