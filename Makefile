ifeq ($(origin WINDOWS), undefined)
  PROG := das
  CC := gcc
else
  PROG := das.exe
  CC := i586-mingw32msvc-gcc
endif

# This makefile could use many improvements, particularly partial compilation
# and auto-dependencies.

# extra / local unversioned includes, if present (e.g. debug CFLAGS)
EXTRAMKS := $(wildcard *.mk)

SRCS := y.tab.c lex.yy.c dasdefs.c das.c instruction.c symbol.c expression.c \
		statement.c dat.c
HDRS := dasdefs.h das.h list.h instruction.h symbol.h expression.h common.h \
		statement.h dat.h output.h
DEPS := $(SRCS) $(HDRS) Makefile $(EXTRAMKS)

CFLAGS := -Wall

-include $(EXTRAMKS)

# TODO: partial compliation, auto dependencies

.SUFFIXES:
#MAKEFLAGS += --no-builtin-rules

$(PROG): $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

y.tab.h y.tab.c: das.y Makefile $(HDRS)
	yacc -d $<

lex.yy.c: das.l $(HDRS) Makefile
	lex --debug $<

clean:
	rm -f $(PROG) y.tab.h y.tab.c lex.yy.c
