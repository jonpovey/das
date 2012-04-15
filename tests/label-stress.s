; crazy labels test

; Later try this:
;:M1		SET a, M1 + 33 - M2
;:M2

:zero
:alsozero
:zero3		SET a,b
:second		JSR nonexist
			SUB a, second
:should_be_3
:redef		XOR [data], b
:redef		XOR x, [y + data]
			JSR redef
:data
:end