namespace CoopFixes::BulletBill {

ncp_asmfunc void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x021470E8, 42)
	MOV     R0, R5
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x021470EC
)");};

} // namespace CoopFixes::BulletBill
