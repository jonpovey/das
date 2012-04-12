#ifndef DASDEFS_H
#define DASDEFS_H

int op2code(char *str);

void yyerror(char *);


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif // #ifndef ARRAY_SIZE

#endif
