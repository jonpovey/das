; test symbol validation errors
; this test passes parsing, tested in 005
; - warnings on unused symbols
; - errors referencing undefined symbols

:unused_zeroA
:unused_zeroB	SET a,b
:second			JSR nonexist		; error undefined symbol.
				SUB a, second
:loop
				XOR [data], b
				XOR x, [y + data]
				SET PC, loop

				.equ circle, circle	; circular reference not noticed at this
									; stage of parsing
:data
:unused_end
