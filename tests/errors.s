; do some things that should generate warnings / errors

errors:
	SET X, PICK + 1		; Illegal +, should be PICK 1
	SET X, PICK			; missing offset
	SET X 0, 1			; PICK form used on non-PICK register

	SET POP, 1			; POP is a-value (right) only
	SET X, PUSH			; PUSH is b-value (left) only
	SET X, O			; O(verflow) register doesn't exist any more

	SET X, Y + 1		; direct reg + const form is never OK
	SET X, SP + 1		; not OK here either
	SET X, [Y + 1]		; this is ok
	SET X, [SP]			; ok, same as PEEK
	SET X, [POP]		; can't indirect from POP, it's already indirect
	SET X, [SP + 1]		; ok, same as PICK 1

; SP++ forms not implemented yet
;
;	SET X, [SP++]		; should be allowed, = POP
;	SET [--SP], Y		; should be allowed, = PUSH
;	SET [SP++], Y		; error, POP is rhs only
;	SET X, [--SP]		; error, PUSH is lhs only

	SET X, [1 + Y + 2]	; error, only one expression per operand
						; could transform this into [(1 + 2) + Y] but won't.
