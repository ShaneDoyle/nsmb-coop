#include <nsmb/game/stage/entity.hpp>
#include <nsmb/core/net.hpp>

#include "coop/CoopActor.hpp"

namespace CoopFixes::Blockhopper {

ncp_over(0x021784B8, 69) const auto vtbl_skipRender = CoopActor::safeSkipRender;

ncp_asmfunc void getClosestPlayerFix_ASM()
{asm(R"(
ncp_jump(0x02177260, 69)
	MOV     R0, R4
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
	B       0x02177264

ncp_over(0x021778A4, 69)
	NOP
	MOV     R0, R4
	BL      _ZN9CoopActor16getClosestPlayerEP10StageActor
ncp_endover()
)");};

ncp_set_call(0x02177450, 69, CoopActor::getClosestPlayer)

// Since there is no "bahp" event in coop, use random jump intervals.
// Return value lower or equal to 0 means "do not jump".
u32 isTimeToJump(StageEntity* self)
{
	if (Game::getPlayerCount() == 1)
	{
		return *rcast<u32*>(0x02088B9C); // bahp
	}
	return (Net::getRandom() & 0xFF) == 0;
}

ncp_repl(0x02177384, 69, "MOV R0, R4")

ncp_asmfunc void timeToJump_ASM()
{asm(R"(
ncp_jump(0x02177388, 69)
	PUSH    {R1}
	BL      _ZN9CoopFixes11Blockhopper12isTimeToJumpEP11StageEntity
	POP     {R1}
	B       0x0217738C
)");};

} // namespace CoopFixes::BalloonBoo
