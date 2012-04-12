#ifndef DAS_H
#define DAS_H

struct num* gen_const(int val);
struct num* gen_label(char *str);
struct value* gen_value(int reg, struct num *num, int indirect);
void gen_instruction(int opcode, struct value *val1, struct value *val2);

extern int das_error;

#endif // DAS_H
