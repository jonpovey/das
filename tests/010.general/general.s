; Test some general things that should all work.
;
; This is not a good tests for isolating specific failures, but more just me
; slinging in various things that I didn't feel like making a zillion tests
; directories for.

; test that .equ sets a value and handles a forward reference
	.equ foo, bar + 10
	DAT foo, foo - 1		; should be 15, 14
	.equ bar, 5
