%{
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

	/*
	Assembly constructs
	-------------------
	Values can be:
		Plain general-purpose register name, one of ABCXYZIJ
		Plain PC, SP or O register
		Literal value									0x10
		Absolute memory address e.g. 					[0x1000]
		Register indirect via gp reg eg 				[A]
		Base 16-bit literal plus reg offset eg 			[0x1000+A]
		PUSH, POP or PEEK ([SP++] forms allowed?)
		Text label: C identifier rules?					myfunc

	All instructions take two arguments except JSR which takes one

	Labels are a colon immediately followed by a valid identifier

	Comments are a semicolon to EOL

	*/

	/*
	describe a value:
	*/

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
	LABELDEF					{ printf("labeldef "); }
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

#if 0
int main(void)
{
	yyparse();
	return 0;
}
#endif
