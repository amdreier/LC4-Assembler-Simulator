.CODE
.ADDR 0

CONST R1, #1		; R1 = 1
ADD R1, R1, R1	; R1 = 2
CONST R2, #3		; R2 = 3
MUL R3, R1, R2	; R3 = 6
TRAP xFF		; HALT



;=================================== OS ====================================;

;; A simple OS that just RTIs back to user code from the default PennSim entry point of x8200.

.OS
.CODE

.ADDR x80FF
HALT
	NOP

.ADDR x8200
.FALIGN
	CONST R7, #0
	RTI		; removes privilege bit

	
