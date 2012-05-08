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
DEPS := $(SRCS) $(HDRS) y.tab.h Makefile $(EXTRAMKS)

CFLAGS := -Wall

-include $(EXTRAMKS)

# TODO: partial compliation, auto dependencies

.SUFFIXES:
#MAKEFLAGS += --no-builtin-rules

$(PROG): $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(SRCS)

ifeq (1,$(USE_YACC))
y.tab.h y.tab.c: das.y Makefile $(HDRS)
	yacc -d $<
else
y.tab.h y.tab.c: y.tab.premade.h y.tab.premade.c
	@echo USE_YACC not set: using premade sources
	cp y.tab.premade.h y.tab.h
	cp y.tab.premade.c y.tab.c
endif

ifeq (1,$(USE_LEX))
lex.yy.c: das.l $(HDRS) Makefile
	lex $<
else
lex.yy.c: lex.yy.premade.c
	@echo USE_LEX not set: using premade sources
	cp lex.yy.premade.c lex.yy.c
endif

.PHONY: clean
clean:
	rm -f $(PROG) y.tab.h y.tab.c lex.yy.c

.PHONY: install
install: $(PROG)
	@if [ -w /usr/bin ]; then \
		echo "Installing to /usr/bin"; \
		INSTALLDIR=/usr/bin; \
	elif [ -w $$HOME/bin ]; then \
		echo "/usr/bin not writable (need sudo?)"; \
		echo "installing to $$HOME/bin"; \
		INSTALLDIR=$$HOME/bin; \
	else \
		echo "Error: /usr/bin and $$HOME/bin not writable. Install where?"; \
		false; \
	fi && cp $(PROG) $$INSTALLDIR
