@CURRENT HEAP (Heap of the heaps)
repl_02065F60:
	LDR     R0, =0x4004000 @DSiMode
	LDRB    R0, [R0]
	ANDS    R0, R0, #1
	LDREQ   R0, =0x23E0000 @DS Heap
	LDRNE   R0, =0x24E0000 @DSi Heap
	BX      LR

@SOUND HEAP
repl_020125F0:
	LDR     R0, =0x4004000 @DSiMode
	LDRB    R0, [R0]
	ANDS    R0, R0, #1
	LDREQ   R0, =0xB0000 @DS Heap
	LDRNE   R0, =0xD0000 @DSi Heap
	BX      LR
	
repl_020125F8:
	LDR     R1, =0x4004000 @DSiMode
	LDRB    R1, [R1]
	ANDS    R1, R1, #1
	LDREQ   R1, =0xB0000 @DS Heap
	LDRNE   R1, =0xD0000 @DSi Heap
	BX      LR