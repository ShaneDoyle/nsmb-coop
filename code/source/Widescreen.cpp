// Original by gamemasterplc
// Updated by TheGameratorT

#include <nsmb_nitro.hpp>
#include <nsmb/game/stage/layout/stagelayout.hpp>
#include <nsmb/game/stage/layout/data/entrance.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/net.hpp>
#include <nsmb/core/wifi.hpp>

namespace Widescreen
{
	u8 enabled[2];
	u8 shouldApply[2];

	bool canApply(u32 playerID)
	{
		if (!enabled[playerID])
			return false;

		if (Scene::currentSceneID == scast<u16>(SceneID::Stage))
		{
			if (scast<u32>(Entrance::spawnEntrance[playerID]->flags & EntranceFlags::SubScreen) != 0)
				return false;

			// Do not apply widescreen if the camera is bound to a screen size smaller than the widescreen size

			fx32 cameraLimitStart = rcast<fx32*>(0x020CAD6C)[playerID];
			fx32 cameraLimitEnd = rcast<fx32*>(0x020CADEC)[playerID];

			if (cameraLimitEnd - cameraLimitStart <= (304 << FX32_SHIFT))
				return false;
		}

		return true;
	}
}

asm(R"(
	fun0200738C = 0x0200738C
)");
extern "C"
{
	void fun0200738C();
}

ncp_call(0x02006F74)
void call_02006F74()
{
	for (u32 consoleID = 0; consoleID < Wifi::getCommunicatingConsoleCount(); consoleID++)
	{
		Widescreen::shouldApply[consoleID] = Widescreen::canApply(consoleID);
	}
	fun0200738C();
}

asm(R"(
.text

Widescreen_shouldApplyForLocalPlayer:
	PUSH    {R0-R1,LR}
	LDR     R1, =_ZN3Net8localAidE
	LDRB    R1, [R1]
	LDR     R0, =_ZN10Widescreen11shouldApplyE
	LDRB    R0, [R0,R1]
	CMP     R0, #0
	POP     {R0-R1,PC}

@ ================ STAGE ================

@ 3D Camera
ncp_jump(0x020ADB38, 0)
	LDR     R2, =_ZN10Widescreen11shouldApplyE
	LDRB    R2, [R2,R1]
	CMP     R2, #0                                          @ Check if Widescreen
	BEQ     skip_scale_stage_camera                         @ Skip If Not Wide
	MOV     R4, R4, ASR#8                                   @ Downshift Camera Right Edge
	LDR     R2, =304                                        @ Scale Value for Camera Right Edge
	MUL     R4, R2                                          @ Scale Camera Right Edge
skip_scale_stage_camera:
	STR     R4, [R12,R1,LSL#2]                              @ Set Camera Right Edge (Keep replaced instruction)
	B       0x20ADB3C                                       @ Return to Game

@ Tileset
ncp_jump(0x02007508)
	BL      0x206232C                                       @ Generate Affine Parameters for Background (Keep replaced instruction)
	BL      Widescreen_shouldApplyForLocalPlayer
	BEQ     0x200750C                                       @ Return to Game If Not Wide
	LDR     R0, =0x2085BD4                                  @ Background Struct Address
	LDR     R0, [R0, #8]                                    @ Get Background Main Affine Parameter Pointer
	LDR     R3, =0x4000020                                  @ Hardware Affine Parameters Register
	LDR     R1, =304                                        @ Scale Value for Affine Backgrounds
	LDR     R2, [R0]                                        @ Get Main Affine A Parameter
	MUL     R2, R1                                          @ Scale Main Affine A Parameter
	MOV     R2, R2, ASR#12                                  @ Convert Main Affine A Parameter to Fixed Point
	STRH    R2, [R3]                                        @ Set Main Affine A Parameter
	LDR     R2, [R0, #8]                                    @ Get Main Affine C Parameter
	MUL     R2, R1                                          @ Scale Main Affine C Parameter
	MOV     R2, R2, ASR#12                                  @ Convert Main Affine C Parameter to Fixed Point
	STRH    R2, [R3, #4]                                    @ Set Main Affine C Parameter
	B       0x200750C                                       @ Return to Game

@ Foreground
ncp_jump(0x020075D0)
	BL      0x206232C                                       @ Generate Affine Parameters for Backgrounds (Keep replaced instruction)
	BL      Widescreen_shouldApplyForLocalPlayer
	BEQ     0x20075D4                                       @ Return to Game If Not Wide
	LDR     R0, =0x2085BD4                                  @ Background Struct Address
	LDR     R0, [R0, #12]                                   @ Get Background Main Affine Parameter Pointer
	LDR     R3, =0x4000030                                  @ Hardware Affine Parameters Register
	LDR     R1, =304                                        @ Scale Value for Affine Backgrounds
	LDR     R2, [R0]                                        @ Get Main Affine A Parameter
	MUL     R2, R1                                          @ Scale Main Affine A Parameter
	MOV     R2, R2, ASR#12                                  @ Convert Main Affine A Parameter to Fixed Point
	STRH    R2, [R3]                                        @ Set Main Affine A Parameter
	LDR     R2, [R0, #8]                                    @ Get Main Affine C Parameter
	MUL     R2, R1                                          @ Scale Main Affine C Parameter
	MOV     R2, R2, ASR#12                                  @ Convert Main Affine C Parameter to Fixed Point
	STRH    R2, [R3, #4]                                    @ Set Main Affine C Parameter
	B       0x20075D4                                       @ Return to Game

@ Sprites
ncp_jump(0x0200DC60)
	LDR     R1, [R1,R2,LSL#2]                               @ Sprite X Position Scale (Keep replaced instruction)
	BL      Widescreen_shouldApplyForLocalPlayer
	BEQ     0x200DC64                                       @ Return to Game If Not Wide
	LDR     R2, =304                                        @ Scale Value for Sprite X Position
	MUL     R1, R2                                          @ Scale Sprite X Position
	MOV     R1, R1, ASR#8                                   @ Downshift Sprite X Position Scale
	B       0x200DC64                                       @ Return to Game

ncp_jump(0x020ADBEC, 0)
	PUSH    {R2,R12}
	LDR     R2, =_ZN10Widescreen11shouldApplyE
	LDRB    R2, [R2,R1]
	CMP     R2, #0                                          @ Check if Widescreen
	BEQ     skip_scale_stage_oam                            @ Skip If Not Wide
	LDR     R2, =304                                        @ Sprites X Scale Value
	MUL     R12, R2                                         @ Scale Sprites X Scale
	MOV     R12, R12, ASR#8                                 @ Downshift Sprites X Scale
skip_scale_stage_oam:
	STR     R12, [R3,R1,LSL#4]                              @ Update Sprites X Scale
	POP     {R2,R12}
	B       0x20ADBF0                                       @ Return to Game

@ 1-1 Background Fix
ncp_jump(0x020B9220, 0)
	BL      Widescreen_shouldApplyForLocalPlayer
	BEQ     skip_bg_1_1_fix
	CMP     R4, #0xE8000
	B       0x020B9224
skip_bg_1_1_fix:
	CMP     R4, #0x100000
	B       0x020B9224

@ ================ WORLDMAP ================

@ 3D Camera
ncp_jump(0x020D1B80, 8)
	BL      Widescreen_shouldApplyForLocalPlayer
	LDREQ   R2, =((256*64)/3)                               @ Aspect Ratio of Normal World Map
	LDRNE   R2, =((304*64)/3)                               @ Aspect Ratio of Wide World Map
	B       0x20D1B84                                       @ Return to Game

@ Pause Textbox
ncp_jump(0x020D8AC4, 8)
	BL      Widescreen_shouldApplyForLocalPlayer
	LDREQ   R1, =((256*25)/16)                              @ X Scale of Pause Screen Textbox in Normal World Map
	LDRNE   R1, =((304*25)/16)                              @ X Scale of Pause Screen Textbox in Wide World Map
	STR     R1, [R2,#0x54]                                  @ Set X Scale of Pause Screen Textbox in World Map
	B       0x20D8AC8                                       @ Return to Game

ncp_over(0x020D8AC8, 8)
	MOV     R1, #0x190
ncp_endover()
)");
