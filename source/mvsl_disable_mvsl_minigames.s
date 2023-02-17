#Disable MvsL and Minigames buttons
hook_020D339C_ov_09:
	LDR     R3, =0x20DBB00
	LDRB    R2, [R3,#0x84]
	CMP     R2, #1
	BEQ     .L2
	CMP     R2, #2
	MOVEQ   R2, #0
	STREQB  R2, [R3,#0x84]
	BX      LR

.L2:
	MOV     R2, #3
	STRB    R2, [R3,#0x84]
	BX      LR