#include "nsmb.h"

void nsub_0218CD60_ov_7B() //mushroom houses (freeze)
{
	asm("BL      0x0211F34C"); //Keep replaced instruction
	asm("MOV     R0, #1");
	asm("BL      GetPtrToPlayerActorByID");
	asm("MOV     R1, #0");
	asm("BL      0x0211F34C");
	asm("B       0x0218CD64"); //Return to code
}

void nsub_0218CE94_ov_7B() //mushroom houses (unfreeze)
{
	asm("BL      0x211F2EC"); //Keep replaced instruction
	asm("MOV     R0, #1");
	asm("BL      GetPtrToPlayerActorByID");
	asm("BL      0x211F2EC");
	asm("B       0x0218CE98"); //Return to code
}

void nsub_0218D1EC_ov_7B() //1up houses (freeze)
{
	asm("BL      0x0211F34C"); //Keep replaced instruction
	asm("MOV     R0, #1");
	asm("BL      GetPtrToPlayerActorByID");
	asm("MOV     R1, #0");
	asm("BL      0x0211F34C");
	asm("B       0x0218D1F0"); //Return to code
}

void nsub_0218D480_ov_7B() //1up houses (unfreeze)
{
	asm("BL      0x211F2EC"); //Keep replaced instruction
	asm("MOV     R0, #1");
	asm("BL      GetPtrToPlayerActorByID");
	asm("BL      0x211F2EC");
	asm("B       0x0218D484"); //Return to code
}

void nsub_0218CF0C_ov_7B()
{
	asm("MOV     R0, #0");
	asm("BL      GetPtrToPlayerActorByID");
	asm("LDR     R1, [R0, #0x780]");
	asm("BIC     R1, R1, #0x20000000");
	asm("STR     R1, [R0, #0x780]");

	asm("MOV     R0, #1");
	asm("BL      GetPtrToPlayerActorByID");
	asm("LDR     R1, [R0, #0x780]");
	asm("BIC     R1, R1, #0x20000000");
	asm("STR     R1, [R0, #0x780]");

	asm("B       0x0218CF24"); //Return to code
}

void nsub_0218E61C_ov_7B()
{
	asm("MOV     R0, #0");
	asm("BL      GetPtrToPlayerActorByID");
	asm("LDR     R1, [R0, #0x780]");
	asm("ORR     R1, R1, #0x20000000");
	asm("STR     R1, [R0, #0x780]");

	asm("MOV     R0, #1");
	asm("BL      GetPtrToPlayerActorByID");
	asm("LDR     R1, [R0, #0x780]");
	asm("ORR     R1, R1, #0x20000000");
	asm("STR     R1, [R0, #0x780]");

	asm("B       0x0218E634"); //Return to code
}