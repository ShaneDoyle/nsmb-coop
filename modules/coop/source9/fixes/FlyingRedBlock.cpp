namespace CoopFixes::FlyingRedBlock {

ncp_asmfunc void getClosestPlayerFix_ASM()
{asm(R"(
ncp_over(0x0215BBE4, 54)
	MOV     R4, R0
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BC48, 54)
	MOV     R4, R0
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	NOP
	NOP
ncp_endover()

ncp_over(0x0215BCCC, 54)
	MOV     R0, R4
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	NOP
ncp_endover()
)");};

} // namespace CoopFixes::FlyingRedBlock
