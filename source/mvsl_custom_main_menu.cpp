#include "nsmb.h"
u8 GlobalFileNo = 0;

//File select to MvsL Menu.
void repl_020D272C_ov_09()
{
	asm("MOV	R0, #0x06");
}

//Bowser Jr. Intro to MvsL Menu.
void repl_020CD700_ov_09()
{
	asm("MOV	R0, #0x06");
}

//Get FileNo selected by the player. (Replace with loadSave later!).
void repl_020D26F4_ov_09(int FileNo)
{
	asm("MOV     R1, #0");
	GlobalFileNo = FileNo;
}