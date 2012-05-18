; first test that ignored sections generate no warnings as this test will
; run with -Wno-ignored-directives. 011 tests with that warning on.
	.text
	.data
	.section
	.globl

; test that the ignore directives stuff doesn't ignore more than it should
; this should cause parse-time errors.
	section blah		; no leading dot, don't ignore
	.sections			; trailing character 's' part of token, no match
