%{
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <stdio.h>

#include "expression.h"
#include "dasdefs.h"
#include "dat.h"
#include "instruction.h"
#include "symbol.h"

#define YYLTYPE LOCTYPE
#define YYLLOC_DEFAULT(Current, Rhs, N) do { \
	if (N) { \
		(Current).line = YYRHSLOC(Rhs, 1).line; \
	} else { \
		(Current).line = YYRHSLOC(Rhs, 0).line; \
	} \
} while (0)

int yylex();

void parse_error(char *str);
%}

%code requires {
#include "output.h"
#define YYLTYPE LOCTYPE
}

%locations
%error-verbose

%initial-action {
	@$.line = 1;
}

%union {
	int  integer;
	char *string;
	struct expr *expr;
	struct operand *operand;
	struct dat_elem *dat_elem;
	struct symbol *symbol;
}

%token <string> SYMBOL LABEL STRING
%token <integer> CONSTANT
%token <integer> REG
%token <integer> OP1 OP2 DAT
%token <integer> OPERATOR
%token <integer> LSHIFT RSHIFT
%token <integer> EQU

%type <symbol> symbol
%type <expr> expr
%type <operand> operand op_expr
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
	| program error '\n'		{ yyerrok; }
	|
	;

line:
	statement
	| label
	| label statement
	;							

label:
	LABEL						{ label_parse(@$, $1); }
	;

statement:
	dat
	| instr
	| EQU symbol ',' expr		{ directive_equ(@$, $2, $4); }
	;

instr:
	OP2 operand ',' operand		{
								operand_set_position($2, OP_POS_B);
								operand_set_position($4, OP_POS_A);
								gen_instruction($1, $2, $4);
								}
	| OP1 operand				{
								operand_set_position($2, OP_POS_A);
								gen_instruction($1, NULL, $2);
								}
	/*| error						{ parse_error("bad instruction"); }*/
	;

operand:
	op_expr
	| '[' op_expr ']'			{ $$ = operand_set_indirect($2); }
	;

op_expr:
	REG							{ $$ = gen_operand($1, NULL, OPSTYLE_SOLO); }
	| expr						{ $$ = gen_operand(REG_NONE, $1, OPSTYLE_SOLO); }
	| REG expr  /* PICK n */	{ $$ = gen_operand($1, $2, OPSTYLE_PICK); }
	| expr '+' REG				{ $$ = gen_operand($3, $1, OPSTYLE_PLUS); }
	| REG '+' expr				{ $$ = gen_operand($1, $3, OPSTYLE_PLUS); }
 /*	| error						{ parse_error("bad op_expr"); } */
	;

expr:
	CONSTANT					{ $$ = gen_const($1); }
	| symbol					{ $$ = gen_symbol_expr($1); }
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

symbol:
	SYMBOL						{ $$ = symbol_parse($1); }
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
	fprintf(stderr, "line %d: parse error: %s\n", yylloc.line, str);
	das_error = 1;
}
