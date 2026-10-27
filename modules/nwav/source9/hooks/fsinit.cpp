/*============================================================\
|  NWAV card read fix.                                        |
|  InitGame's FS_Init setup chooses DMA channel 2 for card    |
|  reads, unless this is a multiboot child, where it passes   |
|  -1 and the CPU copies instead. Streaming reads the card    |
|  all the time, and a DMA read gets interfered with by the   |
|  background HDMA, so the branch to the DMA choice is never  |
|  taken.                                                     |
\============================================================*/

#include "nwav/hooks.hpp"

ncp_repl(0x02009434, "NOP")
