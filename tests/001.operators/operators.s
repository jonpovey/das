; test operator precedence

	DAT (1 + 2 * 3)		- 7
	DAT (1 + -1)		- 0
;	DAT 1 /0			; would cause divide-by-zero error
	DAT ~1				- 0xfffe
	DAT ~-1				- 0
	DAT -~1				- 2
	DAT --1				- 1
	DAT ~~1				- 1
	DAT (10 - 1 + 2) 	- 11
	DAT (10 - (1 + 2))	- 7
	DAT (4 / 2 * 2)		- 4
	DAT (1 + 2 << 3)	- 0x18
	DAT 1 & 1 + 1
	DAT (3 | 4)			- 7
	DAT (1 | 0 & 0)		- 1
	DAT (0 & 0 | 1)		- 1
	DAT (0xf0 ^ 0x0f)	- 0xff
;	DAT ((8 | )7 ^ 0xf) - 8		; parse error
	DAT (8 | 7 ^ 0xf)	- 8
