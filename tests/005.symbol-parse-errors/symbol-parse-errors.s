; test symbol parse errors
; parse-time errors are:
; redefined symbols. symbols can be defined by labels or .equ


:unused_zeroA
:unused_zeroB	SET a,b
:mix			.equ mix, 1			; ERROR redefine on the same line
				.equ explicit, 69
				.equ explicit, 1	; ERROR redefined

:second			JSR nonexist		; error undefined symbol. this is currently
									; a validation time error which is never
									; hit for this test as das aborts after
									; finding parse errors
				SUB a, second
:unused_3
:redef			XOR [data], b
:redef			XOR x, [y + data]	; ERROR label redefined
				JSR redef
				.equ redef, 3		; ERROR redefining (a label)

				.equ circle, circle	; circular reference not noticed at this
									; stage of parsing

:data
:unused_end
