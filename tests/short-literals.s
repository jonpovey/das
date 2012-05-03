; test short literals
	SET a, -300		; long
	SET a, -3		; long
	SET a, -2		; long
	SET a, -1		; short in 1.5
	SET a, 0
	SET a, 1
	SET a, 2
	SET a, 3
	SET a, 4
	SET a, 5
	; ...
	SET a, 0x1d
	SET a, 0x1c
	SET a, 0x1e		; short
	SET a, 0x1f		; long in 1.5
	SET a, 0x20
	SET a, 0x200

; warning cases
	SET a, -0x8000	; fits
	SET a, -0x8001	; wrap to 0
	SET a, 0xffff	; fits
	SET a, 0x10000	; wrap to 0

; test short literal forced long
			SET a, from + 32 - to	; may be short literal
:from		SET a, from + 32 - to	; must be long literal
:to			SET a, from + 32 - to	; may be short literal
:end

; In this case one or other instruction may use short form, but not both.
; Hard to optimise, and which to choose to make short?
; test 0x1e boundary
:a1			SET a, a2 + 32 - b2
:b1
:a2			SET a, a1 + 32 - b1
:b2

; test -1 boundary
:a3			SET a, b4 - a4 - 3
:b3
:a4			SET a, b3 - a3 - 3
:b4
