#pragma once

#include "nsmb_nitro.hpp"

static inline bool Stage_isBossFight() { return *rcast<u32*>(0x020CA8C0) & 0x80000000; }
static inline bool Stage_hasLevelFinished() { return *rcast<u32*>(0x020CA8C0) & 1; }
