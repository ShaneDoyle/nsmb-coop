#pragma once

#include "../vector.h"

// VecFx

#ifdef __cplusplus
// c++

struct VecFx32
{
	fx32 x, y, z;
};

struct VecFx16
{
	fx16 x, y, z;
};

#else
// c

typedef struct
{
	Fx32 x, y, z;
} VecFx32;

typedef struct 
{
	Fx16 x, y, z;
} VecFx16;

#endif

// functions

fx32 VEC_Mag(VecFx32* in);
void VEC_Normalize(VecFx32* in, VecFx32* out);
void VEC_CrossProduct(VecFx32* in_a, VecFx32* in_b, VecFx32* out);
fx32 VEC_DotProduct(VecFx32* in_a, VecFx32* in_b);

void VEC_Fx16Normalize(VecFx16* in, VecFx16* out);
void VEC_Fx16CrossProduct(VecFx16* in_a, VecFx16* in_b, VecFx16* out);
fx32 VEC_Fx16DotProduct(VecFx16* in_a, VecFx16* in_b);