#include "nwav/nwav.hpp"

ncp_call(0x0204F2F0)
void call_0204F2F0()
{
	NNS_SndInit(); // Keep replaced instruction
	NWAV::init();
}
