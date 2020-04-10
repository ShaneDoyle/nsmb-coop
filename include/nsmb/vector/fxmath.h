#pragma once

#include "../vector.h"

// types
typedef s32 fx32;
typedef s16 fx16;

// rounding numbers
#define INT(n)		(s32)((fx32)(n)>>12)
#define FLOOR(n)	((fx32)(n)&((u32)~0xfff))
#define CEIL(n)		FLOOR((fx32)(n)+(u32)0xfff)
#define ROUND(n)	((((fx32)(n)&(u32)0x800)!=0)?CEIL(n):FLOOR(n))

#define HI64(n)		(fx32)((u64)(n)>>32)
#define LO64(n)		(fx32)((u64)(n))

// macro for quick 64-bit square root
#define SQRT64(n)	Sqrt64(LO64(n), HI64(n))

// functions
#ifdef __cplusplus
extern "C" {
#endif

	fx32 FX_Div(fx32 n, fx32 d);
	fx32 Sqrt64(fx32 l, fx32 h);

#ifdef __cplusplus
}
#endif