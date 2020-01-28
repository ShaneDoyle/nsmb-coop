#include "nsmb.h"

//Level start music load (MvsL)
void repl_02152E48_ov_34()
{
	int* MvslMode = (int*)0x02085A84;
	if (*MvslMode == 2)
	{
		Music_ResetVars();
		return;
	}

	bool IsToadHouseLevel = *(int*)0x02085ACC & 0x20;
	if (!IsToadHouseLevel)
		Music_LoadLevelSeqs();
	else
		Music_LoadToadHouseLevelSeqs();

	Music_LoadLevelMusic();
}

//Uncomment if using any entrances hack
void nsub_020FC398_ov_0A()
{
	__asm("CMP     R0, #2");
	__asm("BEQ     0x020FC42C");
	__asm("B       0x020FC3AC");
}

//Prevent music from not playing in MvsL select stage and results scene
void repl_021559B4_ov_34(int R0, int R1)
{
	int* MvslMode = (int*)0x02085A84;
	if (*MvslMode == 2)
		Music_PlaySeq(R0, R1);
	else
	{
		Music_ClearAndLoadSeqWithSoundset(R0, R1);
		Music_PlaySeq(R0, R1);
	}
}

void hook_0215693C_ov_34()
{
	int* MvslMode = (int*)0x02085A84;
	if (*MvslMode == 2)
		return;

	ClearSoundHeap();
	int SeqIDs[4] = { 32, 33, 34, -1 };
	Music_LoadSeqs(SeqIDs);
}

//Allow music to load on view change
void nsub_02118B14_ov_0A()
{
	__asm("CMP     R0, #2");
	__asm("BEQ     0x02118B2C"); //Load stage musics
	__asm("B       0x02118B28");
}

//Allow MvsLMode multicard to store next music to play (entrances and Mega Mario)
void nsub_0212B774()
{
	__asm("CMP     R0, #2");
	__asm("BNE     0x0212B794");
	__asm("B       0x0212B788");
}

//Allow MvsLMode multicard to fetch level musics
void nsub_0201E028()
{
	__asm("LDR     R1, =0x02085A84"); //MvsLMode
	__asm("LDR     R1, [R1]");
	__asm("CMP     R1, #2");
	__asm("LDREQ   R0, =0x0203C6C0;"); //MvsLMusicToPlay
	__asm("ADDEQ   SP, SP, #4");
	__asm("LDREQ   R0, [R0]");
	__asm("LDMEQFD SP!, {PC}");
	__asm("B       0x0201E050");
}

//No bahp points in Mario Vs Luigi (prevents desynchronization)
void nsub_02012584()
{
	__asm("LDR     R0, =0x2085A84 @MvsLMode");
	__asm("LDR     R0, [R0]");
	__asm("CMP     R0, #0");
	__asm("LDREQ   R0, =0x2088B94");
	__asm("BLEQ    0x204DB48");
	__asm("B       0x201258C");
}