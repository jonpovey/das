; test new/All opcodes

	SET B, POP
	;SET B, [SP++]	; later
	SET PUSH, A
	;SET [--SP], A	; later

:foo
	SET B, PEEK
	SET PEEK, A
	SET PICK 1, A
	SET B, PICK foo + 3
	;SET [SP + 3], A				; later
	;:label1 SET [label1 + SP], A	; also later

	SET B, A
	ADD B, A
			
	SUB B, A
			
	MUL B, A
			
	MLI B, A
	DIV B, A
			
	DVI B, A
	MOD B, A
	MDI B, A
	AND B, A
	BOR B, A
	XOR B, A
	SHR B, A
			
	ASR B, A
			
	SHL B, A

	IFB B, A
	IFC B, A
	IFE B, A
	IFN B, A
	IFG B, A
	IFA B, A
	IFL B, A
	IFU B, A
	   
	ADX B, A
			
	SBX B, A

	STI B, A
	STD B, A

	JSR A

	HCF A		; undocumented since spec 1.7, but still exists
	INT A
	IAG A
	IAS A
	;IAP A		; no more IAP, became RFI
	RFI A		; A is ignored
	;RFI		; no-arg shorthand, later

	IAQ A
		 

	HWN A
	HWQ A
		 
	HWI A
