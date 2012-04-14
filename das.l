%{
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
void yyerror(char *);
#include "y.tab.h"
#include "dasdefs.h"
#include "das.h"
static int get_constant(void);
%}

%option yylineno
/* shut up warnings */
%option noinput
%option nounput

letter		[A-Za-z]
digit		[0-9]
hexdigit	[0-9a-fA-F]
labelchar	{letter}|[\.$_]
label		{labelchar}({labelchar}|{digit})*
gpreg		[ABCXYZIJabcxyzij]
xreg		SP|PC|O|POP|PEEK|PUSH|sp|pc|o|pop|peek|push
op2		SET|ADD|SUB|MUL|DIV|MOD|SH[LR]|AND|[BX]OR|IF[ENGB]
op2_lc		set|add|sub|mul|div|mod|sh[lr]|and|[bx]or|if[engb]
op1		JSR|jsr

%%

:{label}	{ yylval.stringval = yytext + 1; return LABELDEF; }
0x{hexdigit}+	return get_constant();
{digit}+	return get_constant();
{gpreg}		{ yylval.intval = str2reg(yytext); return GPREG; }
{xreg}		{ yylval.intval = str2reg(yytext); return XREG; }
{op2}|{op2_lc}	{ yylval.intval = str2op(yytext); return OP2; }
{op1}		{ yylval.intval = str2op(yytext); return OP1; }
{label}		{ yylval.stringval = yytext; return LABEL; }

[,+\[\]\n]	return *yytext;

[ \t]+		;	/* ignore whitespace */
;.*		;	/* comment */

		/* Magic to fix input with missing \n on last line */
<<EOF>> 	{ static int once = 0; return once++ ? 0 : '\n'; }

.		yyerror("Invalid character ");

%%

static int get_constant(void)
{
	yylval.intval = (int)strtol(yytext, NULL, 0);
	return CONSTANT;
}

int get_lineno(void) { return yylineno; }

int yywrap(void)
{
	return 1;
}

void yyerror(char *s)
{
	fprintf(stderr, "line %d: %s\n", yylineno, s);
	das_error = 1;
}
