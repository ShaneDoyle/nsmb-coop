#include <nsmb_nitro.hpp>
#include <nsmb/core/filesystem/file.hpp>
#include <nsmb/core/graphics/2d/font.hpp>
#include "nsmb/extra/log.hpp"

#include "fid.hpp"

// Load the NARC contents from the filesystem instead.

// ncp_over(x) const u32 MGFontNCL_fileID = "message/d_2d_mario_3Dfont_ncl.bin"fid;
ncp_over(0x02018B14) const u32 MGFontNCG_fileID = "message/d_2d_mario_3Dfont_ncg.bin"fid;
ncp_over(0x02026664) const u32 ErrorBmg_fileID = "message/error.bmg"fid;
ncp_over(0x02014A8C) const u32 MainFont_fileID = "message/font_a.NFTR"fid + 131;
ncp_over(0x02014A94) const u32 LoadingFont_fileID = "message/font_b.NFTR"fid + 131;
ncp_over(0x02018B18) const u32 MsgData_fileID = "message/msg_data.bin"fid;

// The fonts load before the boot scene sets the file ID offset, so their IDs include it.
// A soft reset to the title screen sets it earlier, so it must not be applied twice.
ncp_call(0x02014A68)
ncp_call(0x02014A74)
void Font_initMainFonts_loadFontHook(NNSG2dFont* font, u32 fileID)
{
	Font::loadFont(font, fileID - FS::fileIDOffset);
}

// End the archive list before the NARC
// menu_common.narc was removed
// message_common.narc uses above instead
ncp_over(0x020262D0) const u32 FS_Archive_mainGameArchives_END = 47;
