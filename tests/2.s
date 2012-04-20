; test expressions
:zero	DAT zero, zero + 1, zero * 100
:foo	DAT foo * bar, (bar * foo), foo << bar, foo + bar * 3 + 1
:bar	DAT -foo, -0xfff0, ~0xeee1, -10 * -2
:main	SET A, [B + 12 * 3 - foo]

; test short literal forced long
			SET C, from + 30 - to
:from		SET C, from + 30 - to
:to			SET C, from + 30 - to
