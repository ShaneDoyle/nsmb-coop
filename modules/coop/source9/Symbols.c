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
// All symbols prefixed with CoopSym_ to avoid clashing with other modules

#define s(n,v) asm(".globl CoopSym_" #n ";.type CoopSym_" #n ",%function;CoopSym_" #n "=" #v);
#define d(n,v) asm(".globl CoopSym_" #n ";.type CoopSym_" #n ",%object;CoopSym_" #n "=" #v);

s(SledgeBro_tryShakePlayer, 0x02174DE4)
s(StageLayout_volcanoShake, 0x020AD65C)
s(BossController_bindCameraToZone, 0x02142F14)
s(BossController_transitionState, 0x02143550)
s(BossController_switchState, 0x021439EC)
s(BossController_sTransition, 0x02146C08)
s(FinalBossController_transitionState, 0x021480A0)
s(FinalBossController_switchState, 0x02148580)
s(FinalBossController_sTransition, 0x02148AF0)
s(setupFSCacheToUseOverlay55, 0x021726C0)
s(loadCastleModel, 0x020F4660)
s(FinalBowser_loadResources, 0x02138724)
s(Bowser_getBattleState, 0x0213884C)
s(BossSwitch_switchState, 0x0213B1DC)
s(debug_printf, 0x02006370)
s(debug_clear, 0x02005E68)
s(debug_drawTop, 0x02005EB0)
s(debug_drawBottom, 0x020061E4)
s(Stage_switchScene, 0x020A183C)
s(Liquid_doWaves, 0x021646E0)
s(Player_doSpecialPlayerBump, 0x0210A20C)
s(fun0200738C, 0x0200738C)
s(SpawnGrowingEntranceVine, 0x020D0CEC)
s(Stage_zoom, 0x020CADB4)
s(StageLayout_looperScrollBack, 0x020B1510)
s(DrawBottomScreenLives, 0x020BEC60)
s(SetupGraphicsForBottomScreenInStage, 0x020BDAFC)
s(InitSubEngineVBlankStates, 0x02005734)
s(Flagpole_switchState, 0x02130734)
s(Flagpole_updateGoalGrab, 0x0213042C)
s(Flagpole_sPlayerSlide, 0x02132500)
s(fun020CD884, 0x020CD884)
s(Worldmap_onCreate, 0x020CF7C8)
s(StageLayout_setupView, 0x020BBBDC)
s(WarpCannon_sAfterShoot, 0x0217FE60)
s(WarpCannon_switchState, 0x0217F7D4)
s(setSecretExitFromSpriteData, 0x020068F0)
s(MegaDestructionCounter_spawnReward, 0x0215F5FC)
s(MegaDestructionCounter_fun0215FA0C, 0x0215FA0C)
s(LoadBGDatFile, 0x0201ED50)

// Title screen co-op menu
s(TitleScreen_loadFileSelectGraphics, 0x020D2D74)
s(TitleScreen_loadMainMenuGraphics, 0x020D34A0)
s(CommonMenu_loadSubButtonGraphics, 0x020BD604)
s(TextLabel_loadScript, 0x020180C0)
s(TextLabel_unloadScript, 0x020180A4)
s(TextLabel_render, 0x02018060)
s(TextLabel_setMessage, 0x02017FF8)
s(TextLabel_loadFireballIcon, 0x02017B94)
s(TextLabel_updateFireballIcon, 0x02017A90)
s(TextLabel_hideFireballIcon, 0x02017A80)
s(Fader_enableMainScreenFading, 0x02007C44)
s(Fader_startSceneFade, 0x02007BFC)
s(Wifi_loadIconPalette, 0x0200E944)
d(nicknameCache, 0x0208ADCC)
d(vsConsoleCharacters, 0x0208AE54)
d(vsConnectCancelled, 0x02088F30)
d(touchX, 0x0208B4E8)
d(touchValid, 0x0208B4EC)
d(touchPressed, 0x0208B4F0)
d(touchY, 0x0208B4F4)
