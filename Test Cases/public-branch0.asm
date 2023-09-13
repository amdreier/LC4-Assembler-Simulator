.code
	.addr x0
	const r0, x0
	brn FAIL
	brp FAIL
	brnp FAIL
	brz PASS
FAIL:	nop
	nop
PASS:	nop
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

	
