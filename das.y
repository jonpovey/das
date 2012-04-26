%{
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <stdio.h>

#include "common.h"

int yylex();

void yyerror(char *);
int get_lineno(void);
void parse_error(char *str);
%}

%union {
	int  integer;
	char *string;
	struct expr *expr;
	struct operand *operand;
	struct dat_elem *dat_elem;
}

%token <string> SYMBOL LABEL STRING
%token <integer> CONSTANT
%token <integer> GPREG
%token <integer> XREG
%token <integer> OP1 OP2 DAT
%token <integer> OPERATOR
%token <integer> LSHIFT RSHIFT

%type <expr> expr
%type <operand> operand
%type <integer> gpreg
%type <dat_elem> dat_elem datlist

%left '|'
%left '^'
%left '&'
%left LSHIFT RSHIFT
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS '~'

%%

program:
	program line '\n'			{ /*printf("line\n");*/ }
	| program '\n'				/* empty line or comment */
	|
	;

line:
	statement
	| label
	| label statement
	;							

label:
	LABEL						{ label_parse($1); }
	;

statement:
	dat
	| instr
	;

instr:
	OP2 operand ',' operand		{ gen_instruction($1, $2, $4); }
	| OP1 operand				{ gen_instruction($1, NULL, $2); }
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
	| '-' expr %prec UMINUS 	{ $$ = expr_op(UMINUS, NULL, $2); }
	| '~' expr %prec '~'		{ $$ = expr_op('~', NULL, $2); }
	| expr '+' expr				{ $$ = expr_op('+', $1, $3); }
	| expr '-' expr				{ $$ = expr_op('-', $1, $3); }
	| expr '*' expr				{ $$ = expr_op('*', $1, $3); }
	| expr '/' expr				{ $$ = expr_op('/', $1, $3); }
	| expr '^' expr				{ $$ = expr_op('^', $1, $3); }
	| expr '&' expr				{ $$ = expr_op('&', $1, $3); }
	| expr '|' expr				{ $$ = expr_op('|', $1, $3); }
	| expr LSHIFT expr			{ $$ = expr_op(LSHIFT, $1, $3); }
	| expr RSHIFT expr			{ $$ = expr_op(RSHIFT, $1, $3); }
	| '(' expr ')'				{ $$ = expr_op('(', NULL, $2); }
	;

dat:
	DAT datlist					{ gen_dat($2); }
	;

datlist:
	dat_elem
	| dat_elem ',' datlist		{ $$ = dat_elem_follows($1, $3); }
	;

dat_elem:
	expr						{ $$ = new_expr_dat_elem($1); }
	| STRING					{ $$ = new_string_dat_elem($1); }
	/* maybe 8bit char array etc later */
	;

%%

void parse_error(char *str)
{
	// yylineno has advanced since the actual line this was found on
	fprintf(stderr, "line %d: parse error: %s\n", get_lineno() - 1, str);
	das_error = 1;
}
