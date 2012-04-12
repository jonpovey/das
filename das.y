%{
#include <stdio.h>
void yyerror(char *);

%}

%token LABEL LABELDEF
%token CONSTANT
%token GPREG
%token XREG
%token OP1 OP2

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
	program line '\n'			{ printf("line "); }
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
	OP2 value ',' value			{ printf("op2 "); }
	| OP1 value					{ printf("op1 "); }
	| error						{ printf("error "); }
	;

value:
	XREG
	| gpreg
	| num
	| '[' gpreg ']'
	| '[' num ']'
	| '[' num '+' gpreg ']'
	| '[' gpreg '+' num ']'
	|							{ yyerror("bad value "); }
	;

gpreg:	GPREG;

num:
	CONSTANT					{ printf("const "); }
	| LABEL						{ printf("label "); }
	;

%%

int main(void)
{
	yyparse();
	return 0;
}
