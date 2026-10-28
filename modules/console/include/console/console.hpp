#pragma once

#include <nsmb_nitro.hpp>

// Text console drawn on the bottom screen's BG1, by ItzTacos (from NSMB Utils).
//
// Escape sequences inside printed strings:
//   ^c<n>^  switch to palette <n> for the rest of the print call
//   ^l<n>^  draw a full width line with palette <n>
//
// Palettes: 0 white, 1 red, 2 green, 3 yellow, 4 blue, 5 purple, 6 cyan, 7 grey
namespace Console {

enum class Align : u8
{
	Left,
	Center,
	Right
};

constexpr u32 Width = 32;
constexpr u32 Height = 24;

// Characters drawn by the font past the ASCII range
constexpr char LineChar = '\x90';
constexpr char BackChar = '\xF3';

// Saves the bottom screen state it is about to overwrite and takes it over
bool open();
// Gives the bottom screen back as it was before open
void close();
bool isOpen();
// Forgets the console without touching the screen, for when the scene it was on is gone
void discard();

void clear();
// Pushes the text buffer to VRAM, call once per frame after printing
void update();

void print(const char* text, ...);
void printxy(u8 x, u8 y, const char* text, ...);
void gotoXY(u8 x, u8 y);
void align(Align align);
void setColumnOff(u8 col);
// Black by default, open resets it
void setBackdrop(GXRgb color);

} // namespace Console
