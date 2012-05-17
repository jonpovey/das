; test symbol validation errors
; this test passes parsing, tested in 005
; - warnings on unused symbols
; - errors referencing undefined symbols

:unused_zeroA
:unused_zeroB	SET a,b
				JSR nonexist		; error, undefined symbol.
				DAT 1, nowt - 3		; error, undefined
				.equ nused2, undef2	; one unused, the other undefined
:loop
				XOR [data], b
				XOR x, [y + data]
				SET PC, loop

				.equ circle, circle	; circular reference not noticed at this
									; stage of parsing
:data
:unused_end
