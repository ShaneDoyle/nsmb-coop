Nothing_ctor:
	STMFD	SP!, {R4, LR}
	LDR		R0, =0x40C
	BL		NSMB_AllocFromGameHeap
	MOVS	R4, R0
	BEQ		Nothing_ctor_end
	BL		EnemyActor_ctor
	LDR		R0, =Nothing_vtable
	STR		R0, [R4]

Nothing_ctor_end:
	MOV		R0, R4
	LDMFD	SP!, {R4, PC}

Nothing_null:
	BX		LR

Nothing_beforeDraw:
	B		enemyActor_beforeDraw
	BX		LR

Nothing_init:
	STMFD	SP!, {R4, LR}
	MOV		R4, R0

	LDR		R2, =mainHeapHandle
	LDR		R2, [R2]
	MOV		R1, #0x40
	LDR		R3, [R0]
	LDR		R3, [R3, #0x34]
	BLX		R3

	MOV	R0, R4
	BL		Nothing_onCreate

	MOV	R0, #1
	LDMFD	SP!, {R4, PC}

Nothing_onCreate:
	B Base_deleteIt

Nothing_vtable:
	.long Nothing_init
	.long actor_beforeCreate
	.long actor_afterCreate
	.long base_onDelete
	.long actor_beforeDelete
	.long actor_afterDelete
	.long base_onExecute
	.long enemyActor_beforeExecute
	.long enemyActor_afterExecute
	.long base_onDraw
	.long enemyActor_beforeDraw
	.long actor_afterDraw
	.long base_willBeDeleted
	.long base_moreHeapShit
	.long base_createHeap
	.long base_heapCreated
	.long unknown_dtor
	.long unknown_dtorFree
	.long actor_getSomething
	.long actor_incrementSomething
	.long enemyActor_executeState0 @ r0=1 (blarg?)
	.long enemyActor_isInvisible
	.long enemyActor_executeState1
	.long enemyActor_executeState2
	.long enemyActor_executeState3
	.long enemyActor_executeState4
	.long enemyActor_executeState5
	.long enemyActor_executeState6
	.long enemyActor_executeState7
	.long enemyActor_executeState8
	.long enemyActor_executeState9
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long enemyActor_executeAllStates
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null
	.long Nothing_null

Nothing_Profile:
	.int   Nothing_ctor
	.int   0
	.int   0
	.int   0
	.int   0