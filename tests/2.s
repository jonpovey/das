; test short literal forced long
			SET a, from + 32 - to	; may be short literal
:from		SET a, from + 32 - to	; must be long literal
:to			SET a, from + 32 - to	; may be short literal
:end

; In this case one or other instruction may use short form, but not both.
; Hard to optimise, and which to choose to make short?
:a1			SET a, a2 + 32 - b2
:b1
:a2			SET a, a1 + 32 - b1
:b2

:a3			SET a, b4 - a4 - 3
:b3
:a4			SET a, b3 - a3 - 3
:b4
