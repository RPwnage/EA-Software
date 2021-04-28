/////////////////////////////////////////////////////////////////////////////
// GameDataPS3.cpp
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Written and maintained by Paul Pedriana.
/////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <EAIO/PS3/GameDataPS3.h>
#include <sys/memory.h>
#include <sdk_version.h>
#include <string.h>


#if (CELL_SDK_VERSION < 0x280000) // Sony removed support for the GAMEDATA API in SDK 280.


namespace EA
{
namespace IO
{


PS3GameData* PS3GameData::spThis = NULL;


PS3GameData::PS3GameData(const char* pTitleName, const char* pTitleId)
{
	memset(&mStatus, 0, sizeof(mStatus));
	memset(&mParameters, 0, sizeof(mParameters));

	// Title name
	if(!pTitleName)
		pTitleName = "Untitled";
	strcpy(mParameters.title, pTitleName);

	// Title name in other languages
	for(int i = 0; i < CELL_GAMEDATA_SYSP_LANGUAGE_NUM; i++)
		strcpy(mParameters.titleLang[i], pTitleName);

	// Title Id
	// The title Id is used as the name of the directory that will be returned. 
	// Thus "title id" and "directory name" are synonymous.
	if(!pTitleId)
		pTitleId = "TITLE_ID_";
	strcpy(mParameters.titleId, pTitleId);

	// Data version
	strcpy(mParameters.dataVersion, "00000"); // This could be configurable.

	// Flags
	// These used to work but Sony broken them some time around the 200 SDK.
	// mParameters.parentalLevel = CELL_SYSUTIL_GAME_PARENTAL_LEVEL01;
	// mParameters.attribute     = CELL_GAMEDATA_ATTR_NORMAL;
}


PS3GameData::PS3GameData(CellGameDataSystemFileParam* pParam)
{
	mParameters = *pParam;
}


const char* PS3GameData::GetGameDataDirectory(const char* pTitleId, const char* pTitleName)
{
	if(mStatus.gameDataPath[0]) // If already called...
		return mStatus.gameDataPath;

	int result = -1;

	spThis = this;

	if(pTitleId)
		strcpy(mParameters.titleId, pTitleId);

	if(pTitleName)
		strcpy(mParameters.title, pTitleName);

	// The game data feature appears to require a system memory container of
	// at least 3MB, though this is not documented by Sony. It's better if 
	// the user creates the memory container externally and this class uses it.
	if(sys_memory_container_create(&mMemoryContainerId, 3 * 1024 * 1024) == 0) // PS3 defines 1MB as (1024 * 1024).
	{
		result = cellGameDataCheckCreate2(CELL_GAMEDATA_VERSION_CURRENT, 
								mParameters.titleId, CELL_GAMEDATA_ERRDIALOG_NONE, 
								StaticGameDataCallback, mMemoryContainerId);

		sys_memory_container_destroy(mMemoryContainerId);
	}

	if(result == 0)
	{
		strcat(mStatus.gameDataPath, "/");
		return mStatus.gameDataPath;
	}

	mStatus.gameDataPath[0] = 0;
	return NULL;
}


void PS3GameData::StaticGameDataCallback(CellGameDataCBResult* pResult, CellGameDataStatGet* pGetStatus, CellGameDataStatSet* pSetStatus)
{
	if(pGetStatus->isNewData)
	{
		// A new directory will be created at pGetStatus->contentInfoPath.
		pSetStatus->setParam = &spThis->mParameters;
	} 

	// Save the resulting directory paths. 
	// Sony's documentation regarding pGetStatus->contentInfoPath and gameDataPath is senseless.
	// contentInfoPath will be something like: /dev_hdd0/game/TEST12345
	// gameDataPath will be something like: /dev_hdd0/game/TEST12345/USRDIR
	// It appears that Sony wants us to use gameDataPath, despite their samples using contentInfoPath.
	memcpy(&spThis->mStatus, pGetStatus, sizeof(spThis->mStatus));

	// The pGetStatus->sizeKB parameter will hold zero for a newly created directory but otherwise
	// will return the size of the pGetStatus->contentInfoPath directory. A value of -1 means that
	// the directory size is unknown and hasn't been calculated. The pGetStatus->hddFreeSizeKb 
	// variable stores the available space on the hard drive (and thus presumably available to the app).

	pResult->result = CELL_GAMEDATA_CBRESULT_OK; // We could alternatively return CELL_GAMEDATA_CBRESULT_ERR_NOSPACE, CELL_GAMEDATA_CBRESULT_ERR_BROKEN, etc.
}


} // namespace IO

} // namespace EA


#endif // (CELL_SDK_VERSION < 0x280000)









