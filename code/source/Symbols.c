/*
For some reason, GCC does not change BLs to BLXs if a label of an ARM function is directly defined.

If SledgeBro_tryShakePlayer is called from a thumb function, it won't have its BL changed to BLX, causing a crash.

asm(R"(
	SledgeBro_tryShakePlayer = 0x02174DE4
)");
extern "C" {
	void SledgeBro_tryShakePlayer(StageEntity* self, s32 playerID);
}

This is a workaround by forcing the linker to resolve the symbol.
*/

// Here are the symbols not present in the NSMB Code Reference used in this project

#define s(n,v) asm(".globl " #n ";.type " #n ",%function;" #n "=" #v);

s(SledgeBro_tryShakePlayer, 0x02174DE4)
s(StageLayout_volcanoShake, 0x020AD65C)
s(_ZN5Stage9exitLevelEm, 0x020A189C)
s(BossController_bindCameraToZone, 0x02142F14)
s(BossController_transitionState, 0x02143550)
s(BossController_switchState, 0x021439EC)
s(BossController_sTransition, 0x02146C08)
s(FinalBossController_transitionState, 0x021480A0)
s(FinalBossController_switchState, 0x02148580)
s(FinalBossController_sTransition, 0x02148AF0)
s(setupFSCacheToUseOverlay55, 0x021726C0)
s(func20F4660, 0x020F4660)
s(FinalBowser_loadResources, 0x02138724)
s(Bowser_getBattleState, 0x0213884C)
s(BossBattleSwitch_switchState, 0x0213B1DC)
s(debug_printf, 0x02006370)
s(debug_clear, 0x02005E68)
s(debug_drawTop, 0x02005EB0)
s(debug_drawBottom, 0x020061E4)
s(Stage_switchScene, 0x020A183C)
s(Liquid_doWaves, 0x021646E0)
s(fun0200738C, 0x0200738C)
s(SpawnGrowingEntranceVine, 0x020D0CEC)
s(_ZN5Stage9exitLevelEm, 0x020A189C)
s(_ZN5Stage4zoomE, 0x020CADB4)
s(StageLayout_looperScrollBack, 0x020B1510)
s(DrawBottomScreenLives, 0x020BEC60)
s(SetupGraphicsForBottomScreenInStage, 0x020BDAFC)
s(Flagpole_switchState, 0x02130734)
s(Flagpole_updateGoalGrab, 0x0213042C)
s(Flagpole_sPlayerSlide, 0x02132500)
s(fun020CD884, 0x020CD884)
s(Worldmap_onCreate, 0x020CF7C8)
s(StageLayout_setupView, 0x020BBBDC)
s(WarpCannon_sAfterShoot, 0x0217FE60)
s(WarpCannon_switchState, 0x0217F7D4)
