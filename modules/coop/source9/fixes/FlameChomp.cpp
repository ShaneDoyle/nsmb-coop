namespace CoopFixes::FlameChomp {

ncp_asmfunc void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x02175B18, 70)
	MOV     R0, R4
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x02175B1C
)");};

} // namespace CoopFixes::FlameChomp
