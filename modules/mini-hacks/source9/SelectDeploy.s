.text
.syntax unified
.arm
.align 2

// Deploy powerup by pressing SELECT

selectDeployPressed:
	LDR     R1, =_ZN5Input17playerKeysPressedE
	LSL     R3, R6, #1
	LDRH    R1, [R1,R3]
	TST     R1, #4
	BX      LR

ncp_jump(0x020C0548, 0)
	// R6 = playerID
	// R1,R3 safe to use
	ADD     R5, R1, R6 // Keep replaced instruction
	BL      selectDeployPressed
	BNE     0x020C05C8 // Holding select, deploy powerup
	B       0x020C054C // Not holding select, continue

ncp_jump(0x020C0600, 0)
	// R6 = playerID
	// R0,R1,R2,R3 safe to use
	BL      selectDeployPressed
	BNE     0x020C0630 // Holding select, play fail deploy SFX
	LDRB    R0, [R5] // Keep replaced instruction
	B       0x020C0604 // Not holding select, continue
