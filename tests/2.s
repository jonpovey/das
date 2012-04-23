; test short literal forced long
			SET a, from + 33 - to	; may be short literal
:from		SET a, from + 33 - to	; must be long literal
:to			SET a, from + 33 - to	; may be short literal
:end

; In this case one or other instruction may use short form, but not both.
; Hard to optimise, and which to choose to make short?
:a1			SET a, a2 + 33 - b2
:b1
:a2			SET a, a1 + 33 - b1
:b2
