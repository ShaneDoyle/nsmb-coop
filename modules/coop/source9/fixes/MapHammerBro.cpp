namespace CoopFixes::MapHammerBro {

void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x0215DEBC, 54)
	MOV     R0, R5
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x0215DEC0
)");}

} // namespace CoopFixes::MapHammerBro
