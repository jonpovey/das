PROG := das
SRCS := y.tab.c lex.yy.c dasdefs.c das.c instruction.c symbol.c expression.c \
		statement.c dat.c
HDRS := dasdefs.h das.h list.h instruction.h symbol.h expression.h common.h \
		statement.h dat.h output.h
DEPS := $(SRCS) $(HDRS) Makefile

CC := gcc
CFLAGS := -g -Wall

# TODO: partial compliation, auto dependencies

.SUFFIXES:
#MAKEFLAGS += --no-builtin-rules

$(PROG): $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

y.tab.h y.tab.c: $(PROG).y Makefile $(HDRS)
	yacc -d $<

lex.yy.c: $(PROG).l $(HDRS) Makefile
	lex $<
