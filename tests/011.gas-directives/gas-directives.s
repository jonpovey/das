; test handling of gnu assembler directives produced by clang
; das should ignore most of these for the moment.
	SET A, 1
	.text			; parameters? ignore this for now.
	.data			; parameters? ignore this for now.
	.section foo	; parameters? ignore this for now.
	.globl symname	; ignore for now
	.short 3, 4		; interpret as DAT
	.short "blah"	; should one day error, but for now treat as DAT
