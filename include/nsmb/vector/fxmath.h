#pragma once

#include "../vector.h"

// types
typedef s32 Fx32;
typedef s16 Fx16;

// rounding numbers
#define INT(n)		(s32)((Fx32)(n)>>12)
#define FLOOR(n)	((Fx32)(n)&((u32)~0xfff))
#define CEIL(n)		FLOOR((Fx32)(n)+(u32)0xfff)
#define ROUND(n)	((((Fx32)(n)&(u32)0x800)!=0)?CEIL(n):FLOOR(n))

#define HI64(n)		(Fx32)((u64)(n)>>32)
#define LO64(n)		(Fx32)((u64)(n))

// macro for quick 64-bit square root
#define SQRT64(n)	Sqrt64(LO64(n), HI64(n))

// functions
#ifdef __cplusplus
extern "C" {
#endif

	Fx32 FX_Div(Fx32 n, Fx32 d);
	Fx32 Sqrt64(Fx32 l, Fx32 h);

#ifdef __cplusplus
}
#endif