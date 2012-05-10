# extra / local unversioned includes, if present (e.g. debug CFLAGS)
EXTRAMKS := $(wildcard *.mk)
MAKEFILES := Makefile $(EXTRAMKS)
-include $(EXTRAMKS)

SRCDIR := src

ifeq ($(origin WINDOWS), undefined)
  PROG := das
  BUILDDIR := build
  ifneq ($(origin LINUX_BUILD_32BIT), undefined)
    # I run 64-bit but normally build 32-bit so the distributed binaries will
    # work for more people.
    CFLAGS += -m32
    LDFLAGS += -m32
  else
    $(info wibble)
  endif
else
  PROG := das.exe
  CROSS_COMPILE ?= i586-mingw32msvc-
  BUILDDIR := win32_build
endif

CC ?= $(CROSS_COMPILE)gcc
STRIP ?= $(CROSS_COMPILE)strip

OBJDIR := $(BUILDDIR)
DEPDIR := $(BUILDDIR)/dep

# quietness
ifneq (1,$(V))
  override Q=@
else
  override Q=
endif

CSRCS := y.tab.c lex.yy.c dasdefs.c das.c instruction.c symbol.c expression.c \
		statement.c dat.c
CSRCS:=$(addprefix $(SRCDIR)/, $(CSRCS))

#YACCIN  := $(SRCDIR)/das.y
#YACCOUT := $(YACCIN:%.y=$(BUILDDIR)/lex.%.c)
#YACCSRC := $(YACCOUT:%.c=%.o)
#LEXIN   := $(SRCDIR)/das.l
#LEXOUT  := $(LEXIN:$(SRCDIR)/%.l

#CSRCS:=$(CSRCS) $(YACCSRC) $(LEXSRC)
SRCS:=$(CSRCS)
OBJS:=$(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS:=$(SRCS:$(SRCDIR)/%.c=$(DEPDIR)/%.d)

-include $(DEPS)

CFLAGS += -Wall -g

.SUFFIXES:
#MAKEFLAGS += --no-builtin-rules

.DEFAULT_GOAL := $(PROG)

$(SRCDIR)/y.tab.c $(SRCDIR)/y.tab.h: $(SRCDIR)/das.y $(MAKEFILES)
ifeq (1,$(USE_YACC))
	@echo " + YACC $<"
	$(Q)yacc -d -o $@ $<
else
	@echo " + KEEP $@     (USE_YACC not set)"
	$(Q)touch $@
endif

$(SRCDIR)/lex.yy.c: $(SRCDIR)/das.l $(SRCDIR)/y.tab.h $(MAKEFILES)
ifeq (1,$(USE_LEX))
	@echo " +  LEX $<"
	$(Q)lex -o$@ $<
else
	@echo " + KEEP $@     (USE_LEX not set)"
	$(Q)touch $@
endif

$(PROG): $(OBJS) $(LINKERSCRIPT)
	@echo " + LINK $@"
	$(Q)$(CC) $(LDFLAGS) $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(MAKEFILES)
	@echo " +   CC $<"
	@mkdir -p $(dir $@)
	$(Q)$(CC) -c $(CFLAGS) -MD $< -o $@
    # -MD creates both .d and .o in one pass. Now process the .d file
	@mkdir -p $(dir $(DEPDIR)/$*); \
	cp $(OBJDIR)/$*.d $(DEPDIR)/$*.d; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(OBJDIR)/$*.d >> $(DEPDIR)/$*.d; \
	rm -f $(OBJDIR)/$*.d

.PHONY: clean
clean:
	rm -rf $(PROG) $(BUILDDIR)

ifeq ($(origin WINDOWS), undefined)
.PHONY: install
install: $(PROG)
	$(Q)$(STRIP) $(PROG)
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
endif
