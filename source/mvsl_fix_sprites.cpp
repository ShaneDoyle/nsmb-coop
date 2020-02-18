//Fix Volcano BG
void repl_020B6B6C(){asm("MOV     R1, #0");}

//Remove delete range for UnusedSpikeBass (256).
void repl_021728EC_ov_3A(){}

//Fix Rotating Barrel (246) Desync.
void repl_02186F50_ov_60(){asm("MOV     R1, #0");}

//Fix horizontal movement mushroom. (DeleteIfOutOfRange) (PROBABLY DOESN'T WORK)
void repl_0217FDDC_ov_5A(){}

//Fix Boo (DeleteIfOutOfRange)
void repl_02175CA4_ov_47(){}

//Fix Dorie (DeleteIfOutOfRange) Doesn't work.
void repl_021474FC_ov_2F(){ asm("MOV     R0, #0"); }

//Fix rotating carry platform (BooHouse, DeleteIfOutOfRange)
void repl_0218EC4C_ov_76(){}


//Bowser No Skeleton (T Pose).
/*void nsub_021385BC_ov_0D()
{
	asm("B 	0x21385D0");
}
*/