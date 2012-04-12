PROG := das
SRCS := y.tab.c lex.yy.c dasdefs.c das.c
HDRS := dasdefs.h das.h
DEPS := $(SRCS) $(HDRS) Makefile

$(PROG): $(DEPS)
	gcc -o $@ $(SRCS)

y.tab.h y.tab.c: $(PROG).y Makefile $(HDRS)
	yacc -d $<

lex.yy.c: $(PROG).l $(HDRS) Makefile
	lex $<
