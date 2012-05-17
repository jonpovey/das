; test divide by zero
SET A, 1 / 0			; easy, constant
.equ foo, 2
.equ bar, 3
SET A, 1 / (bar - foo - 1)	; a bit fiddlier, still divide-by-zero
