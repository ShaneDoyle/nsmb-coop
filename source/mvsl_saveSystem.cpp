#include "nsmb.h"

//Allow Luigi's lives to be saved and loaded
int repl_02012DB0() { return (GetLivesForPlayer(1) << 16) | GetLivesForPlayer(0); }
void repl_02012E84(int, int Lives)
{
	SetLivesForPlayer(0, Lives & 0xFFFF);
	SetLivesForPlayer(1, Lives >> 16);
}
//Allow Luigi's powerup state to be saved
int repl_02012DE0() { return (GetPowerupForPlayer(1) << 16) | GetPowerupForPlayer(0); }
void nsub_02012EB4() { asm("B 0x02012EBC"); }
void repl_02012EBC(int, int Powerup)
{
	SetPowerupForPlayer(0, Powerup & 0xFFFF);
	SetPowerupForPlayer(1, Powerup >> 16);
}
//Allow Luigi's inventory powerup to be loaded and saved
u8 repl_02012DF0() { return (GetStoredPowerupForPlayer(1) << 4) | GetStoredPowerupForPlayer(0); }
void repl_02012ECC(int, u8 InventoryPowerup)
{
	SetStoredPowerupForPlayer(0, InventoryPowerup & 0xF);
	SetStoredPowerupForPlayer(1, InventoryPowerup >> 4);
}

//Set lives for Luigi too in some LoadSaveAndSetCounters condition
void nsub_02012E6C()
{
	asm("ORRNE   R4, R4, R4, LSL#16");
	asm("STRNE   R4, [R0, #0xC]");
	asm("B       0x02012E70");
}