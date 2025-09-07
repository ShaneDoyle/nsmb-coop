#include <nsmb/game/sound.hpp>

#include "fid.hpp"
#include "coop/CoopStage.hpp"
#include "nwav/nwav.hpp"

ncp_call(0x02006B28)
ncp_thumb void Stage_loadLevelHook(const void* pSrc, u32 offset, u32 szByte)
{
	GX_LoadBGPltt(pSrc, offset, szByte); // Keep replaced instruction

	CoopStage::defaultOnLevelLoad();
}

ncp_call(0x020BB7DC, 0)
ncp_thumb void StageLayout_onCreateHook(s32 seqID)
{
	SND::stopRequestedBGM(seqID); // Keep replaced instruction

	CoopStage::defaultOnLayoutCreate();
}

void StageLayout_onUpdateHook()
{
	CoopStage::defaultOnLayoutUpdate();
}

ncp_asmfunc void StageLayout_onUpdateHookCall_ASM()
{asm(R"(
ncp_jump(0x020BAC24, 0)
	BL      _Z24StageLayout_onUpdateHookv
	LDR     R0, =0x020CA850
	B       0x020BAC28
)");}

// Use NWAV for the cannon sounds to save memory

constexpr u32 marioCannonShootSfxFileID = "coop/SE_VOC_MA_SHOT.nwav"fid;
constexpr u32 luigiCannonShootSfxFileID = "coop/SE_VOC_LU_SHOT.nwav"fid;

ncp_call(0x020F871C, 10)
ncp_thumb void PipeCannon_customPlayShootSound(s32 sfxID, const Vec3* pos)
{
	SND::playSFXUnique(sfxID, pos); // Keep replaced instruction
	NWAV::play(sfxID == 69 ? marioCannonShootSfxFileID : luigiCannonShootSfxFileID);
}

ncp_thumb void WarpCannon_customPlayShootSound(s32 sfxID, const Vec3* pos)
{
	SND::playSFX(sfxID, pos); // Keep replaced instruction
	NWAV::play(sfxID == 68 ? marioCannonShootSfxFileID : luigiCannonShootSfxFileID);
}

ncp_set_call(0x0217F23C, 89, WarpCannon_customPlayShootSound)
ncp_set_call(0x0217F24C, 89, WarpCannon_customPlayShootSound)
