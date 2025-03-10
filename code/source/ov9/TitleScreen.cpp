#ifdef NTR_DEBUG
#include <nsmb/core/entity/scene.hpp>
#endif

#include <nsmb/game/sound.hpp>
#include <nsmb/core/system/save.hpp>
#include <nsmb/core/system/input.hpp>

#include "Widescreen.hpp"

asm(R"(
	TitleScreen_onCreate = 0x020D3570
	fun020CD884 = 0x020CD884
)");
extern "C" {
	s32 TitleScreen_onCreate(void*);
	void fun020CD884(void*);
}

// Skip MvsL and Minigames buttons
ncp_repl(0x020D317C, 9, "ADDNE R1, R1, #3")
ncp_repl(0x020D319C, 9, "SUBNE R1, R1, #3")

#ifndef NTR_DEBUG
ncp_repl(0x020D272C, 9, "MOV R0, #6") // File select to MvsL Menu
#else
ncp_call(0x020D272C, 9)
u32 TitleScreen_getSceneIDAfterFileSelect()
{
	return scast<u32>((Input::getHeldKeys(0) & Keys::Select) ? SceneID::Worldmap : SceneID::VSConnect);
}
#endif

ncp_repl(0x020CD700, 9, "MOV R0, #6") // Bowser Jr. Intro to MvsL Menu
ncp_repl(0x020D3708, 9, "MOV R1, #0") // MvsL returns to Main Menu button 0

s32 TitleScreen_onCreate_ext(void* self)
{
	s32 result = TitleScreen_onCreate(self);

	Widescreen::enabled[0] = Save::optionSave.flags & (1 << 1);

	return result;
}

ncp_repl(0x020DAA94, 9, ".int _Z24TitleScreen_onCreate_extPv")

// Widescreen toggle

ncp_call(0x020D3474, 9)
void TitleScreen_updateHook(void* r0)
{
	fun020CD884(r0); // Keep replaced instruction

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
			Save::writeOptionSave(&Save::optionSave);
		}
	}
}
