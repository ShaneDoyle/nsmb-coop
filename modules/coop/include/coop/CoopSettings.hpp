#pragma once

#include <nsmb_nitro.hpp>

// Rules picked from the pause menu's Options entry by whoever paused the game.
// The menu only reads the pause owner's input, which every console receives,
// so all consoles apply the same changes on the same frame and stay in sync.
namespace CoopSettings {

// The assembly patches index CoopSettings::values by these, keep them in sync
enum Setting : u8
{
	PlayerCollision,	// Players collide with each other at all
	PlayerBump,			// Players push each other away when they touch
	HeadJumpKnockdown,	// Jumping on a player knocks them down instead of only bouncing
	GroundPoundHurt,	// Ground pounding a player damages them instead of only bumping
	MegaCrush,			// Mega players crush the other player
	FireballHit,		// Fireballs hit players instead of passing through

	Count
};

extern u8 values[Count];

NTR_INLINE bool get(Setting setting)
{
	return values[setting];
}

void restoreDefaults();
void onLevelLoad();

void open(u32 ownerID);
// Returns whether the menu took this frame's input, including the frame it closes on
bool update();
bool isOpen();

} // namespace CoopSettings
