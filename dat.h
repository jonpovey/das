#ifndef DAT_H
#define DAT_H
#include "expression.h"

struct dat_elem;
struct dat;

void gen_dat(struct dat_elem *elem);
struct dat_elem* dat_elem_follows(struct dat_elem *a, struct dat_elem *list);
struct dat_elem* new_expr_dat_elem(struct expr *expr);
struct dat_elem* new_string_dat_elem(char *str);



#endif
