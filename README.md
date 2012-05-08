# BlueDAS DCPU-16 Assembler
This is a command-line assembler, written in C, for the DCPU-16 processor;
if you don't know what that is, see http://0x10c.com/

### I don't like reading! Shut up and give me binaries!
Okay, sheesh. [**Binaries for Linux and Windows are here**]
(https://github.com/jonpovey/das/downloads).  
Mac OSX users have to build it themselves, for now.

### I like reading. Tell me about it.
- **Agressively optimised short literals**, even when they depend
  on complicated expressions involving symbols
- Spec 1.7 compliant
- Compile-time constant expressions supporting symbols and most C arithmetic
  and bitwise operators, e.g.

```
	SET A, [(labelA - labelB) * 0xff >> 3]
	DAT label + 2
```

- Strings support C escape sequences e.g. `"\x0f\t1f CHARACTERS\n\0"`
- Supports `.set` or `.equ` for explicit symbols
- Supports `:notch-style` or `traditional:` label syntax
- Little/big endian output switch (default little-endian)
- Accepts lowercase opcodes and register names
- Pretty printed annotated assembly dump shows machine code and optional PC.
  Example showing some short literal optimisation and a combined P-string /
  C-string:

```
0000 :a1            SET A, a2 + 0x20 - b2                   ; 7c01 001f
0002 :b1
0002 :a2            SET A, a1 + 0x20 - b1                   ; fc01
0003 :b2
0003 :pstring       DAT after - cstring - 1                 ; 000a
0004 :cstring       DAT "ten chars!\0"
0004                    ; 0074 0065 006e 0020 0063 0068 0061 0072
000c                    ; 0073 0021 0000
000f :after
```

### Usage
Just run `das` with no arguments to get something like this:

```
BlueDAS DCPU-16 Assembler, version 0.11
Latest and docs at: https://github.com/jonpovey/das

Usage: das [OPTIONS] asmfile

OPTIONS:
  -o outfile         Write binary to outfile, default das-out.bin
  -v, --verbose      Be more chatty (normally silent on success)
  -d, --dump         Dump human-readable listing to stdout
  --dumpfile file    Dump to file instead
  --no-dump-pc       Omit PC column from dump; makes dump a valid source file
  --le               Generate little-endian binary (default big-endian)

The character '-' for files means read/write to stdin/stdout instead.
```

## How do I install it?
`make install` if you're compiling (works for me on Linux + GCC, anything else:
Good luck), or just copy the binary to somewhere in your PATH.

## It looks pretty cool but I wish it could...

Email me or [raise an issue on github](https://github.com/jonpovey/das/issues).
I just might prioritise it.

### Shortcomings / TODO list

- Better syntax error messages!
- Automate tests
- Include files
- Macros
- `.org`, `.align`
- Local symbols
- Relocation and linking; extern and global symbols
- Aim to support llvm-dcpu16

#### Minor things
- Grownup Makefile with auto-dependencies and good stuff.

BlueDAS is written by Jon Povey <jon@leetfighter.com> a.k.a. blueshift /
cheese_magnet, for fun and to learn lex and yacc.  
Comments, patches, abuse welcome.

This project includes GPLv2 parts from the Linux kernel, so I suppose I'll
release it GPLv2.
