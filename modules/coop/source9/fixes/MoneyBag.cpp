namespace CoopFixes::MoneyBag {

void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x0217C400, 80)
	MOV     R0, R4
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x0217C404
)");}

} // namespace CoopFixes::MoneyBag
