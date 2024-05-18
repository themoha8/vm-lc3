; --------------------------------------------------------------------------------------
; this is a program will work if rewrite trap_getc case in main.c						|
; (comment out all and paste sleep(1))													|
; --------------------------------------------------------------------------------------

.ORIG x3000

		LEA R0, HELLO_STR
		PUTs
		; r3 = -27 (ESC KEY)
		ADD R1, R1, #-10
		ADD R1, R1,	#-10
		ADD R1, R1, #-7

WHILE	LDI R0, KBSRPtr		; key status
		BRz SLEEP
		LDI R0, KBDRPtr		; data from key
		; subtraction
		ADD R2, R0, R1		; add r0 + (-27) r3 
		BRz QUIT
		ADD R2, R0, #0		; save r0
		LEA R0, ENTER_STR
		PUTs
		ADD R0, R2, #0		; restore r0
		OUT
		AND R2, R2, #0		; xor r2, r2
		ADD R0, R2, #10		; "\n"
		OUT

SLEEP	GETC ; sleep 2 sec
		; jmp to while label
		ADD R5, R5, 0
		BRz WHILE

QUIT	HALT

HELLO_STR .STRINGZ "Hello! Enter a characters\n"
ENTER_STR .STRINGZ "\nYou entered: "
; This is necessary to access in memory location located above one byte
KBSRPtr .FILL xFE00
KBDRPtr .FILL xFE02
.END
