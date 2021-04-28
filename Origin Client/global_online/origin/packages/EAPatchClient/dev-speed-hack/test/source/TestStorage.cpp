/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestStorage.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Client.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/URL.h>
#include <EAPatchClient/PatchBuilder.h>
#include <UTFInternet/HTTPServer.h>
#include <UTFInternet/Allocator.h>
#include <EATest/EATest.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAEndian.h>
#include <MemoryMan/MemoryMan.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAFileUtil.h>
#include <eathread/eathread_thread.h>


#if defined(EA_PLATFORM_PS3)
    // We use PS3 Game Data for storing persistent data. Another option for storing data is the System Cache Utility,
    // but that's explicitly for caches and could be wiped at any time by the system. However, the system cache is 
    // much easier to use and for some purposes it could be better for an application than Game Data is.

    #include <sysutil/sysutil_common.h>
    #include <sysutil/sysutil_sysparam.h>
    #include <sysutil/sysutil_gamecontent.h>
    #include <cell/sysmodule.h>


    // GetPS3GameDataLocation
    // 
    // Gets the location of game data for the given game; creates it if not already present.
    // Takes title information as input, and returns a directory path and free space.
    // Free space calculations need to be rounded up to 1024 for each file. So if your data
    // will have two files of 5 bytes each, they will use a total of 2048 bytes of free game data space.
    // 
    // Need to add libsysutil_stub.a and libsysutil_game_stub.a to the app's linked library list.
    // Relevant Sony links:
    //     https://ps3.scedev.net/docs/ps3-en,Development_Process-Overview,File_Configuration_and_Creation_of_Game_Content/#HDD_Boot_Games
    //     https://ps3.scedev.net/docs/ps3-en,Development_Process-Overview,Application_Configuration/1
    //     https://ps3.scedev.net/forums/thread/145193/
    //     https://ps3.scedev.net/forums/thread/75124/
    //     https://ps3.scedev.net/support/issue/49224
    //     https://ps3.scedev.net/forums/thread/218375/
    // 
    // Example usage:
    //     char8_t  gameDataDirectory[EA::IO::kMaxPathLength];
    //     uint64_t freeSpace;
    //
    //     GetPS3GameDataLocation("Game X", "GameX-28931", "1.0", "GameX28931Data", gameDataDirectory, freeSpace);
    //
    bool GetPS3GameDataLocation(const char8_t* pTitle, const char8_t* pTitleId, const char8_t* pTitleVersion, const char8_t* pTitleDirName, 
                                char8_t* pGameDataDirectory, uint64_t& freeSpace)
    {
        bool returnValue = false;
        int  result = 0;

        // Early exit if we've already executed this function previous and saved the results.
        static char8_t gameDataDirectoryCached[CELL_GAME_PATH_MAX + 1] = {};

        if(gameDataDirectoryCached[0])
        {
            EA::StdC::Strlcpy(pGameDataDirectory, gameDataDirectoryCached, EA::IO::kMaxPathLength);
            return true;
        }

        // cellSysmoduleLoadModule must be explicitly loaded by the application.
	    result = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);
        EAPATCH_ASSERT_FORMATTED(result == CELL_OK, ("CELL_SYSMODULE_SYSUTIL_GAME load failure: 0x%08x", result));

        if(result == CELL_OK)
        {
            // cellGameBootCheck is a required function call. It tells you if your app is an HDD boot or 
            // a disc boot, and tells you how much space exists for game content, including free space.
            // It allows you to make space estimates before doing any actual work writing data.
            unsigned int        appType = 0;                                        // CELL_GAME_GAMETYPE_DISC or CELL_GAME_GAMETYPE_HDD
            unsigned int        appAttributes = 0;                                  // CELL_GAME_ATTRIBUTE_DEBUG, etc.
            CellGameContentSize size;                                               // Size of game content and available size of content.
            char                hddBasedDataDirName[CELL_GAME_DIRNAME_SIZE] = {};   // Filled in only for CELL_GAME_GAMETYPE_HDD games. CELL_GAME_DIRNAME_SIZE == 32
            char                contentInfoPath[CELL_GAME_PATH_MAX];                // CELL_GAME_PATH_MAX == 128


            result = cellGameBootCheck(&appType, &appAttributes, &size, hddBasedDataDirName);
            EAPATCH_ASSERT_FORMATTED(result == CELL_GAME_RET_OK, ("cellGameBootCheck failure: 0x%08x", result));

            // You must follow up a call to cellGameBootCheck (or cellGameDataCheck) with cellGameContentPermit, 
            // else the game data kernel system will be locked and not allow further access. The resulting paths
            // will be paths to where the parent dir for your app binary (contentInfoPath), and your app binary (gameDataDirectoryCached) are. 
            result = cellGameContentPermit(contentInfoPath, gameDataDirectoryCached);
            EAPATCH_ASSERT_FORMATTED(result == CELL_GAME_RET_OK, ("cellGameContentPermit failure: 0x%08x", result));



            // cellGameDataCheck must be called before using game data. Additionally it tells you if 
            // there is already game data present, and if so what its size is.
            // The name used for dirName must be different between Disc-based games and
            // HDD-based games. You can ensure this by using a made-up name for the former
            // and use the dirName returned by cellGameBootCheck for the latter.
            // The cellGameDataCheck documentation states that for CELL_GAME_GAMETYPE_DISC dirName 
            // be NULL, but it's wrong and it needs to be NULL only for the case that you use 
            // CELL_GAME_GAMETYPE_DISC instead of CELL_GAME_GAMETYPE_GAMEDATA.
            // cellGameDataCheck can return CELL_GAME_ERROR_BROKEN if the dirName you pass doesn't
            // match the CELL_GAME_GAMETYPE_ argument (e.g. was created as a different type previously).
            // You *must* call cellGameContentPermit soon after cellGameDataCheck regardless of its return value, 
            // as the PS3 kernel puts itself in a state that locks some things until the call is made. 
            // For more information, refer to the PS3 Technical Note "Conflicts between System Utilities".

            const bool hddGame = (appType == CELL_GAME_GAMETYPE_HDD);
            memset(&size, 0, sizeof(size));
            result = cellGameDataCheck(CELL_GAME_GAMETYPE_GAMEDATA, hddGame ? hddBasedDataDirName : pTitleDirName, &size);
            EAPATCH_ASSERT_FORMATTED((result == CELL_GAME_RET_OK) || (result == CELL_GAME_RET_NONE), ("cellGameDataCheck failure for %s: 0x%08x", gameDataDirectoryCached, result));

            if(result == CELL_GAME_RET_OK) // If the gameData already exists...
            {
                // If not calculated already (it takes time to calculage)... we trigger a calculation of it.
                // This calculation isn't actually necessary unless you need to know the existing size of the game content.
		        if(size.sizeKB == CELL_GAME_SIZEKB_NOTCALC) 
                {
			        result = cellGameGetSizeKB(&size.sizeKB);
                    EAPATCH_ASSERT_FORMATTED(result == CELL_GAME_RET_OK, ("cellGameGetSizeKB failure for %s: 0x%08x", gameDataDirectoryCached, result));
		        }
            }
            else
            {
                CellGameSetInitParams params;
                memset(&params, 0, sizeof(params));
                strcpy(params.title,   pTitle);
                strcpy(params.titleId, pTitleId);
                strcpy(params.version, pTitleVersion);

                // For a disc-based game, this will be an entirely different directory than the directory that
                // resulted from cellGameContentPermit after cellGameBootCheck. The resulting contentInfoPath and
                // gameDataDirectoryCached are not the directories you want to use, as they are temporary names with random
                // GUID-like characters in them. You need to follow up with a call to cellGameContentPermit to 
                // get the actual directory paths that will be used.
                result = cellGameCreateGameData(&params, contentInfoPath, gameDataDirectoryCached);
                EAPATCH_ASSERT_FORMATTED(result == CELL_GAME_RET_OK, ("cellGameCreateGameData failure: 0x%08x", result));
            }


            // The cellGameContentPermit function is required for usage of game data, even if cellGameCreateGameData
            // was called and returned you the same two paths. Apparently, if you write files before calling cellGameContentPermit,
            // they will be written, but they aren't 'registered with the system.' I'm still trying to understand that, based on the PS3 documentation.
            // The gameDataDirectoryCached is the path you want to use to populate with game data. 
            result = cellGameContentPermit(contentInfoPath, gameDataDirectoryCached);
            EAPATCH_ASSERT_FORMATTED(result == CELL_GAME_RET_OK, ("cellGameContentPermit failure: 0x%08x", result));
            
            // Now we can use gameDataDirectoryCached.
            EA::StdC::Strlcat(gameDataDirectoryCached, "/", EAArrayCount(gameDataDirectoryCached));
            EA::StdC::Strlcpy(pGameDataDirectory, gameDataDirectoryCached, EA::IO::kMaxPathLength);

            returnValue = (result == CELL_GAME_RET_OK);

            cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_GAME);
        }

        return returnValue;
    }


    // DestroyPS3GameData
    //
    // You normally wouldn't want to call this function unless you wanted to nuke the game data.
    //
    bool DestroyPS3GameData(const char8_t* pTitleDirName)
    {
        bool returnValue = false;

        // cellSysmoduleLoadModule must be explicitly loaded by the application.
	    int result = cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);
        EAPATCH_ASSERT_FORMATTED(result == CELL_OK, ("CELL_SYSMODULE_SYSUTIL_GAME load failure: 0x%08x", result));

        if(result == CELL_OK)
        {
            // The dirName passed to cellGameDeleteGameData is name originally passed to cellGameDataCheck.
            // This function deletes all the game data from disc as well as the record of its presence. You can always
            // manually delete individual files from the game data directory after gaining access to it. The user can
            // also delete a application's game data by using the PS3 system menu.

            result = cellGameDeleteGameData(pTitleDirName);
            EAPATCH_ASSERT_FORMATTED((result == CELL_GAME_RET_OK) || (result == (int)CELL_GAME_ERROR_NOTFOUND), ("cellGameDeleteGameData failure: 0x%08x", result));

            cellSysmoduleUnloadModule(CELL_SYSMODULE_SYSUTIL_GAME);
        }

        return returnValue;
    }


    int TestStoragePS3()
    {
        int nErrorCount = 0;

        #if (EAIO_VERSION_N >= 21702) // If EA::IO::GetPS3GameDataNames is available...
            // The GetPS3GameDataLocation code below that the application called SetPS3GameDataNames on startup.
            // If you are writing your own version of this then you don't necessarily need to call SetPS3GameDataNames and
            // GetPS3GameDataNames like we do in this app. It's only important that you pass the correct values to 
            // GetPS3GameDataLocation. It turns out that the version number string is documented by Sony as irrelevant.
            // See the EAIO SetPS3GameDataNames function for an explanation of these strings, or see the Sony docs at:
            // https://ps3.scedev.net/docs/ps3-en,Game_Content-Reference,CellGameSetInitParams/1
            
            char8_t title[CELL_GAME_SYSP_TITLE_SIZE];
            char8_t titleId[CELL_GAME_SYSP_TITLEID_SIZE];
            char8_t version[CELL_GAME_SYSP_VERSION_SIZE];
            char8_t dirName[CELL_GAME_PATH_MAX];

            EA::IO::GetPS3GameDataNames(title, titleId, version, dirName);
            EATEST_VERIFY(title[0] && titleId[0] && version[0] && dirName[0]);

            char8_t  gameDataDirectory[EA::IO::kMaxPathLength];
            uint64_t freeSpace;
            bool     bResult = GetPS3GameDataLocation(title, titleId, version, dirName, gameDataDirectory, freeSpace);

            EATEST_VERIFY(bResult && gameDataDirectory[0]);

            // At this point the gameDataDirectory will be present and persisted between sessions. However, the user
            // has the power to delete this data from the system menu, and the OS has the power to delete this data
            // when it needs to make space or uninstall the application.
        #endif

        return nErrorCount;
    }

#endif


#if defined(EA_PLATFORM_XENON) // XBox 360

    ///////////////////////////////////////////////////////////////////////////
    // Relevant reading from the XBox 360 doc .chm file:
    //      Developing with Xbox 360 Unified Storage 
    //      Using Storage
    //      Designing and Implementing an In-Game Store 
    ///////////////////////////////////////////////////////////////////////////

    bool GetXenonStorage()
    {
        using namespace EA::Patch;

        bool     returnValue = false;
        DWORD    dwResult;
        DWORD    userIndex = 0;                         // 0-3
        uint64_t requiredStorageSize = 4 * 1024 * 1024; // 4 MB for this sample. This is a variable value.

        // Microsoft doesn't want you to use the cache drive for game content caching if the game is already 
        // run from the hard drive. This doesn't matter though if we are creating new content on the cache drive.
        // The code below is supposedly the conventional way Microsoft recommends to check for an HD game.
        DWORD dwLicenseMask;
        const bool hddGame = (XContentGetLicenseMask(&dwLicenseMask, NULL) == ERROR_SUCCESS);
        EA_UNUSED(hddGame);


        // How to choose a device through the GUI.
        // Use XShowNuiDeviceSelectorUI instead if the application is a Kinect-based application.
        // This function will block unless called with overlapped.
        HANDLE      hEventOverlappedComplete = CreateEvent(NULL, FALSE, FALSE, NULL);
        XOVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        overlapped.hEvent = hEventOverlappedComplete;
        DWORD deviceID = 0;
        ULARGE_INTEGER requiredSize; requiredSize.QuadPart = requiredStorageSize;
        dwResult = XShowDeviceSelectorUI(userIndex, XCONTENTTYPE_PUBLISHER, XCONTENTFLAG_MANAGESTORAGE, 
                                        requiredSize, &deviceID, &overlapped);
        EAPATCH_ASSERT(dwResult == ERROR_IO_PENDING);
        if(dwResult == ERROR_IO_PENDING)
        {
            dwResult = XGetOverlappedResult(&overlapped, NULL, TRUE);
            if(dwResult == ERROR_SUCCESS)
            {
                // We have the deviceID.
            }
        }
        CloseHandle(hEventOverlappedComplete);


        // How to mount the utility drive (cache:\)
        // It can be mounted only once for an application.
        dwResult = XMountUtilityDrive(FALSE, 16384, 65536*4);
        const bool hddMounted = (dwResult == ERROR_SUCCESS);

        // XContentCreateEnumerator / XEnumerate tell you what all the content files are.
        DWORD         dwContentFlags       = 0; // XCONTENTFLAG_NODEVICE_TRANSFER;
        HANDLE        hEnumeration         = NULL;
        DWORD         dwRequiredBufferSize = 0;     // This is in bytes, not elements.
        XCONTENT_DATA data[16]             = {};    // The max required value here depends on the app. We'lre using XCONTENTTYPE_PUBLISHER as opposed to XCONTENTTYPE_SAVEDGAME and so the max value is usually app-determined. 

        dwResult = XContentCreateEnumerator(userIndex, XCONTENTDEVICE_ANY /*deviceID*/, XCONTENTTYPE_PUBLISHER /*XCONTENTTYPE_SAVEDGAME*/, dwContentFlags,
                                                   EAArrayCount(data), &dwRequiredBufferSize, &hEnumeration);

        // dwResult can also be ERROR_NO_SUCH_USER if userIndex is currently invalid. In that case somebody needs to be signed in on the machine.
        EAPATCH_ASSERT((dwResult == ERROR_SUCCESS) || (dwResult == ERROR_NO_MORE_FILES));
        EAPATCH_ASSERT(dwRequiredBufferSize == sizeof(data));

        DWORD dwDataCount = 0;
        dwResult = XEnumerate(hEnumeration, data, sizeof(data), &dwDataCount, NULL);
        EAPATCH_ASSERT((dwResult != ERROR_SUCCESS) && (dwResult != ERROR_NO_MORE_FILES));

        if((dwResult != ERROR_SUCCESS) && (dwResult != ERROR_NO_MORE_FILES))
        {
            DWORD dwExtendedError = XGetOverlappedExtendedError(NULL);
            EA_UNUSED(dwExtendedError);
        }

        CloseHandle(hEnumeration);


        // How to get info about a device id. e.g. space, readable name, XCONTENTDEVICETYPE_HDD, etc.
        // DWORD XContentGetDeviceData(XCONTENTDEVICEID DeviceID, PXDEVICE_DATA pDeviceData);


        // dwDataCount contains the count of valid items in our data array.
        for(DWORD i = 0; i < dwDataCount; i++)
        {
            // dwContentType should be XCONTENTTYPE_PUBLISHER (0x00000003), because we used XCONTENTTYPE_PUBLISHER above.
            // szDisplayName is the name of the content to display in a GUI.
            // szFileName is the file path to the content. Apparently it's not 0-terminated for the case that it uses all its space, so you need to copy it and terminate it yourself.
            char fileNameTemp[XCONTENT_MAX_FILENAME_LENGTH + 1];
            memcpy(fileNameTemp, data[i].szFileName, XCONTENT_MAX_FILENAME_LENGTH);
            fileNameTemp[XCONTENT_MAX_FILENAME_LENGTH] = 0;

            const String sDescription(String::CtorSprintf(), "Device id: %u, Content type: %u, Name: %ls, File name: %s", 
                                    data[i].DeviceID, data[i].dwContentType, data[i].szDisplayName, fileNameTemp);

            EAPATCH_TRACE_MESSAGE(sDescription.c_str());

            // We can use the given deviceID if we want.
            // deviceID = data[i].DeviceID;
        }

        // For save games and other player-specific content, you must call XContentGetCreator and check the
        // result to make sure the current player matches the conten'ts creating player. It shouldn't be 
        // required for user-independent game data.

        // XContentCreate opens existing content discovered using the XEnumerate function or 
        // creates new content, such as a saved game. It creates not a single file, but a file
        // system of the name you provide.
        // contentData.szDisplayName refers to user-readable name of the volume. Relevant only if you plan on displaying it to the user as some choice. e.g. "Game 001"
        // contentData.szFileName refers to the name of the volume file on disk. It should be unique. e.g. "Game001"
        // The root name is an app-provided name that names the created or opened volume, for the purpose of specifying file paths. e.g. "gamedata" as in gamedata:\Game001.bin

        XCONTENT_DATA contentData;
        memset(&contentData, 0, sizeof(contentData));
        contentData.DeviceID      = deviceID; // This needs to be a device id from one of the enumerations above, or one returned by a call to XShowDeviceSelectorUI.
        contentData.dwContentType = XCONTENTTYPE_PUBLISHER;
        EA::StdC::Strlcpy(contentData.szDisplayName, L"Game 001", EAArrayCount(contentData.szDisplayName));
        EA::StdC::Strlcpy(contentData.szFileName,     "game001",  EAArrayCount(contentData.szFileName));

        DWORD dwDisposition = 0;    // Tells us if the file previously existed (XCONTENT_CREATED_NEW vs XCONTENT_OPENED_EXISTING)
        dwResult = XContentCreate(userIndex, "gamedata", &contentData, XCONTENTFLAG_OPENALWAYS, &dwDisposition, NULL, NULL);
        EAPATCH_ASSERT(dwResult == ERROR_SUCCESS);

        // Now we can open and create files within the "gamedata:\" file system.
        // ...

        XContentClose("gamedata", NULL);

        if(hddMounted)
            XUnmountUtilityDrive();

        return returnValue;
    }


    int TestStorageXenon()
    {
        int nErrorCount(0);

        // GetXenonStorage();

        return nErrorCount;
    }

#endif



int TestStorageMemory()
{
    int nErrorCount(0);

    //

    return nErrorCount;
}





///////////////////////////////////////////////////////////////////////////////
// TestStorage
///////////////////////////////////////////////////////////////////////////////

int TestStorage()
{
    int nErrorCount(0);

    #if defined(EA_PLATFORM_PS3)
        nErrorCount += TestStoragePS3();
    #endif
    #if defined(EA_PLATFORM_XENON)
        nErrorCount += TestStorageXenon();
    #endif
    nErrorCount += TestStorageMemory();

    return nErrorCount;
}




