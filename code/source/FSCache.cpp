#include <nsmb/core/filesystem/cache.hpp>
#include <nsmb/extra/log.hpp>

// Also free textures of models loaded in overlays

static FS::Cache::CacheEntry* overlayLoad3dSetupCacheEntry = nullptr;

ncp_repl(0x02009EB0, "MOV R0, R5; MOV R1, #1")

ncp_call(0x02009EB8)
void FS_Cache_Internal_loadFileToOverlay_AT_setup3DFile_CALL(FS::Cache::CacheEntry* cacheEntry, bool unloadTextures)
{
	overlayLoad3dSetupCacheEntry = cacheEntry;
	FS::Cache::Internal::setup3DFile(cacheEntry->data, unloadTextures);
	overlayLoad3dSetupCacheEntry = nullptr;
}

ncp_call(0x02009E60)
void FS_Cache_Internal_setup3DFile_AT_Memory_reallocate_CALL(Heap* heap, void* ptr, u32 newSize)
{
	if (overlayLoad3dSetupCacheEntry != nullptr)
	{
		u32 removedSize = (overlayLoad3dSetupCacheEntry->size - newSize) & 0xFFFFFFF0;

		*rcast<u32*>(&FS::Cache::overlayFileDest) -= removedSize;
		FS::Cache::overlayFileSize += removedSize;

#ifdef NTR_DEBUG
		if (removedSize != 0)
			Log() << "INFO: Released " << removedSize << " texture bytes from overlay. " << FS::Cache::overlayFileSize << " bytes free.\n";
#endif
		return;
	}

	heap->reallocate(ptr, newSize);
}
