#ifndef DASDEFS_H
#define DASDEFS_H

int str2op(char *str);
char* op2str(int op);

int str2xreg(char *str);
char* reg2str(int reg);

void yyerror(char *);

typedef unsigned short u16;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif // #ifndef ARRAY_SIZE

#endif
