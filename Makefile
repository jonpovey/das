PROG := das
SRCS := y.tab.c lex.yy.c dasdefs.c das.c
HDRS := dasdefs.h das.h list.h
DEPS := $(SRCS) $(HDRS) Makefile

CFLAGS := -Wall

.SUFFIXES:
#MAKEFLAGS += --no-builtin-rules

$(PROG): $(DEPS)
	gcc $(CFLAGS) -o $@ $(SRCS)

y.tab.h y.tab.c: $(PROG).y Makefile $(HDRS)
	yacc -d $<

lex.yy.c: $(PROG).l $(HDRS) Makefile
	lex $<
