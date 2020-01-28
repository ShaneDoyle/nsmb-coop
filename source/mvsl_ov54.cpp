#include "nsmb.h"

void nsub_0200E290() { asm("B 0x0200E2B8"); } //Always load overlay 54 instead of 52
void nsub_0200E1E4() { asm("B 0x0200E20C"); } //Always unload overlay 54 instead of 52

int repl_02013840() { return 0; } //Prevent ov52 call (some dtor)
int repl_02013964() { return 0; } //Prevent ov52 call (scene dtor free)
int repl_02013A90() { return 0; } //Prevent ov52 call (scene dtor)
int repl_020A2FF8() { return 0; } //Prevent ov52 call (stage dtor)
int repl_020FC7A0_ov_0A() { return 0; } //Prevent ov52 call (end of level text dtor)

int repl_020A306C_ov_00() { return 0; } //Prevent ov52 call (MvsL NARC unmount)