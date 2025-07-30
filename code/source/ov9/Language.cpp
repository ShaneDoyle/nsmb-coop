#if GAME_LANGUAGE_JP

#include <nsmb/core/graphics/2d/oam.hpp>

GXOamAttr TitleScreen_logoOamSubtitleAttrs[] = {
	OAM::getOBJAttr(0-88, -8, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 97, 0, 0, 0),
	OAM::getOBJAttr(32-88, -8, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 99, 0, 0, 0),
	OAM::getOBJAttr(64-88, -8, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 101, 0, 0, 0),
	OAM::getOBJAttr(96-88, -8, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 103, 0, 0, 0),
	OAM::getOBJAttr(128-88, -8, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_32x16, GX_OAM_COLOR_16, 105, 0, 0, 0),
	OAM::getOBJAttr(160-88, -8, 0, GX_OAM_MODE_NORMAL, false, GX_OAM_EFFECT_NONE, GX_OAM_SHAPE_16x16, GX_OAM_COLOR_16, 107, 0, 0, 0xFFFF)
};

ncp_repl(0x020CD12C, 9, ".int TitleScreen_logoOamSubtitleAttrs")
ncp_repl(0x020DBA5C, 9, ".int TitleScreen_logoOamSubtitleAttrs")

#endif
