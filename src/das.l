%{
/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#include <stdarg.h>

void yyerror(char *s, ...);
#include "y.tab.h"
#include "dasdefs.h"
#include "output.h"
static int get_constant(void);

#define YY_USER_ACTION yylloc.line = yylineno;

%}

%option yylineno
/* shut up warnings */
%option noinput
%option nounput

ws			[ \t]
letter		[A-Za-z]
digit		[0-9]
hexdigit	[0-9a-fA-F]
symbolchar	{letter}|[\.$_]
symbol		{symbolchar}({symbolchar}|{digit})*
gpreg		[ABCXYZIJabcxyzij]
xreg		SP|PC|EX|POP|PEEK|PUSH|PICK|sp|pc|ex|pop|peek|push|pick
sp_pop		\[{ws}*(SP|sp){ws}*\+\+{ws}*\]
sp_push		\[{ws}*--{ws}*(SP|sp){ws}*\]
op2			SET|ADD|SUB|MUL|MLI|DIV|DVI|MOD|MDI|AND|[BX]OR|SH[LR]|ASR|IF[BCENGALU]|ADX|SBX|ST[ID]
op2_lc		set|add|sub|mul|mli|div|dvi|mod|mdi|and|[bx]or|sh[lr]|asr|if[bcengalu]|adx|sbx|st[id]
op1			JSR|HCF|INT|RFI|IA[GSQ]|HW[NQI]
op1_lc		jsr|hcf|int|rfi|ia[gsq]|hw[nqi]

/* temporary fixup for clang, ignore these: */
ignored_directive	text|data|section|globl

%%

\.{ignored_directive}({ws}.*)?	{
						if (!outopts.no_warn_ignored)
							loc_warn(yylloc, "ignoring directive");
					}

\.set|\.equ			return EQU;
\.text.*			; /* sections generated for gnu as */
\.data.*			; /* sections generated for gnu as */
\.section.*			; /* sections generated for gnu as */
\.globl.*			; /* global symbols generated for gnu as */
:{symbol}			{ yylval.string = yytext + 1; return LABEL; }
{symbol}:			{
						yylval.string = yytext;
						yytext[strlen(yytext) - 1] = 0;
						return LABEL;
					}
0x{hexdigit}+		return get_constant();
{digit}+			return get_constant();
 /* */
{sp_pop}			{ yylval.integer = REG_POP; return REG; }
{sp_push}			{ yylval.integer = REG_PUSH; return REG; }
 /* */
{gpreg}|{xreg}		{ yylval.integer = str2reg(yytext); return REG; }
{op2}|{op2_lc}		{ yylval.integer = str2opcode(yytext); return OP2; }
{op1}|{op1_lc}		{ yylval.integer = str2opcode(yytext); return OP1; }
DAT|dat|\.short		{ return DAT; }
{symbol}			{ yylval.string = yytext; return SYMBOL; }
\"(\\.|[^\\"])*\"	{ yylval.string = yytext; return STRING; }
\<\<				return LSHIFT;
\>\>				return RSHIFT;

[-,+*/|&^()~\[\]\n]	return *yytext;

[ \t\r]+	;		/* ignore whitespace and DOS line endings */
;.*			;		/* comment */

					/* Magic to fix input with missing \n on last line */
<<EOF>>				{ static int once = 0; return once++ ? 0 : '\n'; }

.					yyerror("invalid character '%c'", *yytext);

%%

static int get_constant(void)
{
	yylval.integer = (int)strtol(yytext, NULL, 0);
	return CONSTANT;
}

int yywrap(void)
{
	return 1;
}

void yyerror(char *s, ...)
{
	va_list ap;
	va_start(ap, s);

	fprintf(stderr, "line %d: Error: ", yylineno);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	das_error = 1;
}
