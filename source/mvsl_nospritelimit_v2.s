@Replaces calcFileId
nsub_02009394:
	LDR     R1, =0x2085A84 @IsMvsLMode
	LDR     R1, [R1]
	CMP     R1, #2 @If MvsLMode == 2 (Download Play)
	MOVEQ   R1, #0x10000 @Recover replaced instruction
	BEQ     0x2009398 @Goto untouched code

	MOV     R0, R0, LSL#16
	MOV     R0, R0, LSR#16
        
	@If file is NARC-only, lets pray the right NARC is loaded...
	TST     R0, #0x8000
	BXNE    LR
        
	LDR     R1, =0x02085D04
	LDRSH   R1, [R1]
	ADD     R0, R0, R1
	MOV     R0, R0, LSL#16
	MOV     R0, R0, LSR#16
	BX      LR

@Force Tileset 2 to be loaded if MvsLMode is not DLP
repl_020B39C8:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20B39FC
	BX      LR
repl_020B3A80:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20B3A9C
	BX      LR
repl_020AE018:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20AE058
	BX      LR
repl_020AE070:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20AE0A4
	BX      LR
repl_020AE184:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20AE1A0
	BX      LR
repl_020AE1B8: @I have no idea what this is?
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20AE24C
	BX      LR
        
repl_020AF9EC:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	MOVNE   R0, #0
	MOVEQ   R0, #1
	BX      LR

@Free something. This is probably good.
repl_020B3C4C:
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x20B3C70
	BX      LR

@Disable spawning of initial VS Battle Star if spriteset 16 value is not 9.
nsub_0209C4E0_ov_00:
	LDR     R5, =0x208B000
	LDR     R5, [R5,#0x19C]
	LDRB    R5, [R5,#0xF]
	@R5 = Spriteset 16 value
	CMP     R5, #9
	BLEQ    0x20A0B64
	B       0x209C4E4

@Force all player animations to be loaded (plnovs) if not DLP
nsub_02020728:
	CMP     R0, #2
	BEQ     0x2020748 @If DLP jump through code that loads singleplayer animations.
	B       0x202073C @Else load the singleplayer animations.

@==============
@ Overlay 53 STUFF
@ Since we arent using it we should really only load it in Download Play mode.
@ Otherwise bye bye free RAM.

@Delete the hardcoded load overlay calls if not DLP.
repl_021535BC_ov_34:
repl_021535C4_ov_34:
repl_021535CC_ov_34:
repl_021535D4_ov_34:
repl_021535DC_ov_34:
	STMFD   SP!, {LR}
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	BLEQ    0x2009510 @LoadOverlayOrPanic
	LDMFD   SP!, {PC}

@Delete ov53:createOv53Heap() call if not DLP.
repl_021535EC_ov_34:
	STMFD   SP!, {LR}
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x215CC9C @CreateOv53Heap
	LDMFD   SP!, {PC}

@Delete the hardcoded unload overlay calls if not DLP.
repl_02152C34_ov_34:
repl_02152C3C_ov_34:
repl_02152C44_ov_34:
repl_02152C4C_ov_34:
repl_02152C54_ov_34:
	STMFD   SP!, {LR}
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	BLEQ    0x20094B8 @UnloadOverlay
	LDMFD   SP!, {PC}

@Delete ov53:destroyOv53Heap() call if not DLP.
repl_02152C2C_ov_34:
	STMFD   SP!, {LR}
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	BLEQ    0x215CC70 @DestroyOv53Heap
	LDMFD   SP!, {PC}
        
@Patch ov53:getPtrToOv53Heap() calls if not DLP.
@If result is 0, it will just use the default heap, which _should_ be fine.
repl_021535F0_ov_34:
repl_02153600_ov_34:
	STMFD   SP!, {LR}
	LDR     R0, =0x2085A84 @MvsLMode
	LDR     R0, [R0]
	CMP     R0, #2
	MOVNE   R0, #0
	BLEQ    0x215CC60 @GetPtrToOv53Heap
	LDMFD   SP!, {PC}

@=============
@NARC STUFF
@Since we are not using some NARCs for 2Cards we should only load them in DLP mode.
@Otherwise bye bye free RAM.

repl_02018B58: @LoadNarcForLevel
repl_021535FC_ov_34: @Dat_2d.narc
repl_0215360C_ov_34: @Dat_enemy.narc
	STMFD   SP!, {LR}
	LDR     R2, =0x2085A84 @MvsLMode
	LDR     R2, [R2]
	CMP     R2, #2
	BLEQ    0x2009190 @loadArchiveById
	LDMFD   SP!, {PC}

@==============
@ANIMATION STUFF
@Some animations are not loaded for Download Play so we replace those with loaded similars.

@Fix slope slide animation for DLP
repl_0210F020_ov_0A:
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	MOVEQ   R1, #16 @DLP
	MOVNE   R1, #20 @2Cards
	BX      LR
	
@Fix vine climbing animation for DLP
repl_0210D984_ov_0A:
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	MOVEQ   R1, #2 @DLP
	MOVNE   R1, #144 @2Cards
	BX      LR

@Fix vine started climbing animation for DLP
repl_0210D634_ov_0A:
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	MOVEQ   R1, #5 @DLP
	MOVNE   R1, #145 @2Cards
	BX      LR

@Fix vine standing animation for DLP
repl_0210D584_ov_0A:
	BNE     .IsIDLEinVine
	BX      LR
.IsIDLEinVine:
	LDR     R5, =0x2085A84 @MvsLMode
	LDR     R5, [R5]
	CMP     R5, #2
	MOVEQ   R5, #5 @DLP
	MOVNE   R5, #146 @2Cards
	BX      LR

@Fix fence up/down animation for DLP
repl_0210D978_ov_0A:
	BNE     .IsYMovingInFence
	BX      LR
.IsYMovingInFence:
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	MOVEQ   R1, #2 @DLP
	MOVNE   R1, #112 @2Cards
	BX      LR

@Fix fence left/right animation for DLP
repl_0210D97C_ov_0A:
	BEQ     .IsXMovingInFence
	BX      LR
.IsXMovingInFence:
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	MOVEQ   R1, #2 @DLP
	MOVNE   R1, #117 @2Cards
	BX      LR

@Fix fence standing animation for DLP
repl_0210D578_ov_0A:
	BEQ     .IsIdleInFence
	BX      LR
.IsIdleInFence:
	LDR     R5, =0x2085A84 @MvsLMode
	LDR     R5, [R5]
	CMP     R5, #2
	MOVEQ   R5, #5 @DLP
	MOVNE   R5, #113 @2Cards
	BX      LR

@Fix fence hit animation for DLP
repl_0210D094_ov_0A:
	LDR     R1, =0x2085A84 @MvsLMode
	LDR     R1, [R1]
	CMP     R1, #2
	MOVEQ   R1, #18 @DLP
	MOVNE   R1, #116 @2Cards
	BX      LR