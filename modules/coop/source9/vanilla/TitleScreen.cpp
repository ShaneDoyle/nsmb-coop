#include <nsmb/game/sound.hpp>
#include <nsmb/game/stage/misc.hpp>
#include <nsmb/game/titlescreen/camera.hpp>
#include <nsmb/core/entity/scene.hpp>
#include <nsmb/core/graphics/2d/oam.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/system/input.hpp>

#include "coop/Coop.hpp"

#ifdef MODULE_WIDESCREEN
#include "widescreen/widescreen.hpp"
#endif

extern "C" {
	void CoopSym_fun020CD884(void*);
}

namespace Coop::Vanilla::TitleScreen {

// #ifndef NTR_DEBUG
// ncp_repl(0x020D272C, 9, "MOV R0, #6") // File select to MvsL Menu
// #else
// ncp_call(0x020D272C, 9)
// u32 TitleScreen_getSceneIDAfterFileSelect()
// {
// 	return scast<u32>((Input::getHeldKeys(0) & Keys::Select) ? SceneID::Worldmap : SceneID::VSConnect);
// }
// #endif

// ncp_repl(0x020CD700, 9, "MOV R0, #6") // Bowser Jr. Intro to MvsL Menu
// ncp_repl(0x020D3708, 9, "MOV R1, #0") // MvsL returns to Main Menu button 0



GXOamAttr coopButtonTextAttrs[] = {
	OAM::getOBJAttr(428, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 128, 1, 0, 0x0000),
	OAM::getOBJAttr(460, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 132, 1, 0, 0x0000),
	OAM::getOBJAttr(492, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 136, 1, 0, 0x0000),
	OAM::getOBJAttr(524, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 212, 1, 0, 0x0000), // ext
	OAM::getOBJAttr(556, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLORMODE_16, 216, 1, 0, 0x0000), // ext
	OAM::getOBJAttr(429, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 128, 1, 0, 0x0000),
	OAM::getOBJAttr(461, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 132, 1, 0, 0x0000),
	OAM::getOBJAttr(493, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 136, 1, 0, 0x0000),
	OAM::getOBJAttr(525, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLORMODE_16, 212, 1, 0, 0x0000), // ext
	OAM::getOBJAttr(557, 249, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLORMODE_16, 216, 1, 0, 0x0000), // ext
	OAM::getOBJAttr(404, 248, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLORMODE_16, 30, 1, 0, 0x0000), // wifi icon
	OAM::getOBJAttr(96, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(64, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(32, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(0, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(480, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(448, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(416, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 2, 1, 0, 0x0000),
	OAM::getOBJAttr(384, 240, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x32, GX_OAM_COLORMODE_16, 0, 1, 0, 0x0000),
	OAM::getOBJAttr(96, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(64, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(32, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(0, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(480, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(448, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(416, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 156, 1, 0, 0x0000),
	OAM::getOBJAttr(384, 15, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x8, GX_OAM_COLORMODE_16, 154, 1, 0, 0xFFFF),
};

ncp_repl(0x020DAA38, 9, ".int _ZN4Coop7Vanilla11TitleScreen19coopButtonTextAttrsE")
ncp_repl(0x020DAA3C, 9, ".int 0x020DB56C")

ncp_call(0x020D272C, 9)
u32 getSceneIDAfterFileSelect()
{
	return scast<u32>(Coop::isActive ?
		SceneID::VSConnect :
		SceneID::Worldmap);
}

ncp_asmfunc void modifyButtons()
{asm(R"(
// Button 1
ncp_jump(0x020D2F60, 9)
	LDR     R1, =_ZN4Coop8isActiveE
	MOV     R2, #0
	STRB    R2, [R1]
	B       0x020D2F70 // Open file select

// Button 2
ncp_jump(0x020D2F64, 9)
	LDR     R1, =_ZN4Coop8isActiveE
	MOV     R2, #1
	STRB    R2, [R1]
	B       0x020D2F70 // Open file select

// Button 3
ncp_jump(0x020D2F68, 9)
	LDR     R1, =_ZN4Coop8isActiveE
	MOV     R2, #0
	STRB    R2, [R1]
	B       0x020D2F90 // Switch to VS Connect
)");}

ncp_asmfunc void returnSelectedButton()
{asm(R"(
ncp_jump(0x020D3708, 9)
	LDR     R2, =_ZN4Coop8isActiveE
	LDR     R2, [R2]
	CMP     R2, #0
	MOVNE   R1, #1
	MOVEQ   R1, #2
	B       0x020D370C
)");}

#ifdef MODULE_WIDESCREEN

void TitleScreen_updateOrtho()
{
	fx32 cameraWidth = Widescreen::enabled[0] ? (304 << FX32_SHIFT) : (256 << FX32_SHIFT);
	Stage::cameraWidth[0] = cameraWidth;

	register TitleScreenCamera* r0 asm("r0");
	r0->setOrtho(Stage::cameraHeight[0], 0, 0, cameraWidth);
}

ncp_set_call(0x020D3D10, 9, TitleScreen_updateOrtho)

ncp_repl(0x020D54DC, 9, "CMP R0, #0x150000") // Allow Bowser Jr. and Peach to go offscreen when widescreen

// Widescreen toggle

ncp_call(0x020D3474, 9)
void TitleScreen_updateHook(void* r0)
{
	CoopSym_fun020CD884(r0); // Keep replaced instruction

	u16 held = Input::consoleKeys[0].held;

	if ((held & Keys::L) && (held & Keys::R))
	{
		if (Input::consoleKeys[0].pressed & Keys::X)
		{
			if (Widescreen::enabled[0])
			{
				Widescreen::enabled[0] = false;
				Save::optionSave.flags &= ~(1 << 1);
				SND::playSFX(230);
			}
			else
			{
				Widescreen::enabled[0] = true;
				Save::optionSave.flags |= (1 << 1);
				SND::playSFX(231);
			}
			TitleScreen_updateOrtho();
			Save::writeOptionSave(&Save::optionSave);
		}
	}
}

// TODO: widescreen titlescreen logo, lighting strike fade and luigi

#endif

} // namespace Coop::Vanilla::TitleScreen
