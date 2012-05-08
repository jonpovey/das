; test label errors

:zero
:alsozero
:zero3		SET a,b
:second		JSR nonexist		; error undefined symbol
			SUB a, second
:should_be_3
:redef		XOR [data], b
:redef		XOR x, [y + data]	; error label redefined
			JSR redef
:data
:end