/*
 * Copyright 2012 Jon Povey <jon@leetfighter.com>
 * Released under the GPL v2
 */
#ifndef DASDEFS_H
#define DASDEFS_H

typedef unsigned short u16;

struct reg {
	char *name;
	u16  valbits;
};

int str2opcode(char *str);
char* opcode2str(int op);

int str2reg(char *str);
char* reg2str(int reg);
u16 reg2bits(int reg);

void yyerror(char *);


#define ERR_ON(x) ({ int r = !!(x); \
	if (r) { \
		fprintf(stderr, "%s:%d ERR_ON(%s)\n", __FILE__, __LINE__, #x); \
		das_error = 1; \
	} \
	r; \
})

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif // #ifndef ARRAY_SIZE

#endif
