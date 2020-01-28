#include "nsmb/nsbtx.h"

NSBTX::NSBTX()
{
	this->alpha = 0xFF;
	this->color = WHITE;
	this->light = 0;
	this->center = Vec2(0);
	this->size = Vec2(0);
	this->offset = Vec2(0);
}

void NSBTX::Ctor()
{
	this->alpha = 0xFF;
	this->color = WHITE;
	this->light = 0;
	this->size = Vec2(0);
	this->center = Vec2(0);
	this->offset = Vec2(0);
}

void* NSBTX::operator new(size_t size)
{
	void* temp = NSMB_AllocFromGameHeap(size);
	return temp;
}

void NSBTX::Init(u32 fileID, u32 tex, u32 pal)
{
	this->tex.file = (u8*)NNS_G3dGetTex(nFS_GetPtrToCachedFile(fileID));

	NSBTX_GetTextureParams(this->tex.file, tex, &this->tex.info.texparam);
	NSBTX_GetTexturePalBase(this->tex.file, pal, &this->tex.info.palbase);
}

inline void vec3(int x, int y, int z)
{
	GFX_VERTEX16 = (y << 16) | (x & 0xFFFF);
	GFX_VERTEX16 = (z & 0xFFFF);
}

inline void texcoord2(int x, int y)
{
	GFX_TEX_COORD = (y << 16) | (x & 0xFFFF);
}

void NSBTX::Draw()
{
	G3_LoadMtx43((void*)0x02085B20);
	this->DrawNoMTX();
}

void NSBTX::Draw(void* mtx)
{
	G3_LoadMtx43(mtx);
	this->DrawNoMTX();
}

void NSBTX::DrawNoMTX()
{
	MATRIX_TRANSLATE = this->position.x + 0x8000;
	MATRIX_TRANSLATE = this->position.y + 0x8000;
	MATRIX_TRANSLATE = this->position.z;

	glRotateXi(this->rotation.x);
	glRotateYi(this->rotation.y);
	glRotateZi(this->rotation.z);
	
	MATRIX_SCALE = this->scale.x * 0x40;
	MATRIX_SCALE = this->scale.y * 0x40;
	MATRIX_SCALE = this->scale.z * 0x40;

	MATRIX_CONTROL = GL_TEXTURE;
	MATRIX_IDENTITY = 0;
	MATRIX_CONTROL = GL_MODELVIEW;

	GFX_POLY_FORMAT = POLY_CULL_BACK | POLY_ALPHA(this->alpha >> 3) | 0x800 | 0x3000;

	GFX_TEX_FORMAT = this->tex.info.texparam;
	GFX_PAL_FORMAT = this->tex.info.palbase;

	GFX_COLOR = HEXRGB15(this->color);

	GFX_BEGIN = 1;

	u32 x = this->size.x;
	u32 y = this->size.y;

	u32 cx = this->center.x;
	u32 cy = this->center.y;

	u32 k = 0x40;

	u32 xP = (x - cx) * k;
	u32 xN = -cx * k;
	u32 yP = (y - cy) * k;
	u32 yN = -cy * k;

	x = (x + this->offset.x) * 0x10;
	y = (y + this->offset.y) * 0x10;

	texcoord2(x, 0);
	vec3(xP, yP, 0);

	texcoord2(0	, 0);
	vec3(xN, yP, 0);

	texcoord2(0	, y);
	vec3(xN, yN,0);

	texcoord2(x, y);
	vec3(xP,yN,	0);

	GFX_END = 0;
}