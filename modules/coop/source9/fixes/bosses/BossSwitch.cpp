#include <nsmb/game/stage/entity3danm.hpp>

#include "coop/CoopStage.hpp"
#include "coop/CoopPlayer.hpp"

namespace CoopFixes::BossSwitch {

struct PTMF
{
	bool (*func)(StageEntity*);
	u32 adj;
};

extern "C" {
	bool CoopSym_BossSwitch_switchState(StageEntity* self, PTMF ptmf);
}

fx32 linkedPlayerCameraX = 0;

ncp_thumb Player* onBowserDead(Player* linkedPlayer)
{
	if (Game::getPlayerCount() == 1)
		return linkedPlayer;

	CoopPlayer::beginBossDefeatCutsceneCoop(linkedPlayer, true);
	return linkedPlayer;
}

ncp_thumb void afterHitState_beforeUpdate(StageEntity* self)
{
	linkedPlayerCameraX = Stage::cameraX[self->linkedPlayerID];
}

// Store the player ID that hit the switch
ncp_over(0x0213B2DC, 13) /* max over: 0x30 bytes, current: 0x2C bytes */
ncp_asmfunc void storeTriggerPlayer()
{asm(R"(
	LDRB    R2, [R1,#0x11C]
	CMP     R2, #1
	BXNE    LR
	LDRB    R1, [R1,#0x11E]
	STRB    R1, [R0,#0x11E]
	ADD     R0, R0, 0x400
	LDRH    R1, [R0,#0xA6]
	CMP     R1, #0
	MOVEQ   R1, #1
	STRHEQ  R1, [R0,#0xAE]
	BX      LR
)");}

ncp_jump(0x0213A7AC, 13)
ncp_asmfunc void jump_0213A7AC_ov13()
{asm(R"(
	BL      _ZN4Game9getPlayerEl
	BL      _ZN9CoopFixes10BossSwitch12onBowserDeadEP6Player
	B       0x0213A7B0
)");}

ncp_jump(0x0213A53C, 13)
ncp_asmfunc void jump_0213A53C_ov13()
{asm(R"(
	PUSH    {R1}
	MOV     R0, R5
	BL      _ZN9CoopFixes10BossSwitch26afterHitState_beforeUpdateEP11StageEntity
	POP     {R1}
	LDRSH   R0, [R1,#0xA2]
	B       0x0213A540
)");}

// Use the stored player ID
ncp_repl(0x0213A718, 13, "LDRB R0, [R5,#0x11E]")
ncp_repl(0x0213A7A8, 13, "LDRB R0, [R5,#0x11E]")
ncp_repl(0x0213A8AC, 13, "LDRB R0, [R5,#0x11E]")
ncp_repl(0x0213AF14, 13, "LDRB R0, [R5,#0x11E]")

ncp_set_call(0x0213A6F8, 13, CoopStage::setZoomAll_impl)
ncp_set_call(0x0213A784, 13, CoopStage::setZoomAll_impl)
ncp_set_call(0x0213A7A4, 13, CoopStage::setZoomAll_impl)
ncp_set_call(0x0213A914, 13, CoopStage::setZoomAll_impl)
ncp_set_call(0x0213AA8C, 13, CoopStage::setZoomAll_impl)
ncp_set_call(0x0213ABC8, 13, CoopStage::setZoomAll_impl)

ncp_repl(0x0213AD10, 13, ".int _ZN9CoopFixes10BossSwitch19linkedPlayerCameraXE")

} // namespace CoopFixes::BossSwitch
