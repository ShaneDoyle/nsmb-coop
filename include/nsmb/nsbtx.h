#pragma once

#include "nsmb/gx.h"
#include "nsmb/heap.h"
#include "nsmb/misc.h"
#include "nsmb/nfs.h"
#include "nsmb/nns.h"
#include "nsmb/nsmbtypes.h"
#include "nsmb/vector.h"
#include <nds/arm9/videoGL.h>
#include <nds/ndstypes.h>

#ifdef __cplusplus
class NSBTX
{
public:
	CompTex tex;
	Vec3 position;
	Vec3 scale;
	S16Vec3 rotation;
	Vec2 offset;
	Vec2 size;
	Vec2 center;
	u32 color;
	u8 alpha;
	u8 light;

	//////////

	NSBTX();

	void Ctor();
	void Init(u32 fileId, u32 texNo, u32 palNo);
	void Draw();
	void Draw(void* mtx);

	void* operator new(size_t size);

private:
	void DrawNoMTX();
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

	void NSBTX_GetTextureParams(void* texture, u32 texNo, void* texparam);
	void NSBTX_GetTexturePalBase(void* texture, u32 palNo, void* palbase);

#ifdef __cplusplus
}
#endif