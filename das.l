%{
void yyerror(char *);
#include "y.tab.h"
#include "dasdefs.h"
int opcode(char *);
%}

%option yylineno

letter		[A-Za-z]
digit		[0-9]
label		{letter}({letter}|{digit}|_)*
gpreg		[ABCXYZIJabcxyzij]
xreg		SP|PC|O|POP|PEEK|PUSH
op2		SET|ADD|SUB|MUL|DIV|MOD|SH[LR]|AND|[BX]OR|IF[ENGB]
op1		JSR

%%

;.*\n		return '\n';
^:{label}	return LABELDEF;
(0x)?{digit}+	{
		yylval = atoi(yytext);
		return CONSTANT;
	}
{gpreg}		{ yylval = *yytext; return GPREG; }
{xreg}		return XREG;
{op2}		{ yylval = op2code(yytext); return OP2; }
{op1}		return OP1;
{label}		return LABEL;

[,+\[\]\n]	return *yytext;

[ \t]+		;	/* ignore whitespace */

.		yyerror("Invalid character ");

%%


int yywrap(void)
{
	return 1;
}

void yyerror(char *s)
{
	fprintf(stderr, "line %d: %s\n", yylineno, s);
}