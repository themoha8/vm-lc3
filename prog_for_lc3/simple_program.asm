.ORIG x3000

LEA R0, HELLO_STR
PUTs
GETC
ADD R1, R0, #0				; save r0 in r1
LEA R0, ENTER_STR
PUTs
ADD R0, R1, #0				; restore
OUT
AND R1, R1, #0				; xor r1, r1
ADD R0, R1, #10				; "\n"
OUT
HALT
HELLO_STR .STRINGZ "Hello! Enter a character: "
ENTER_STR .STRINGZ "\nYou entered: "
.END