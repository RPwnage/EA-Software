/////////////////////////////////////////////////////////////////////////////
// GameDataPS3.h
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Written and maintained by Paul Pedriana.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_GAMEDATAPS3_H
#define EAIO_GAMEDATAPS3_H


#include <EABase/eabase.h>
#include <sdk_version.h>

#if (CELL_SDK_VERSION < 0x280000) // Sony removed support for the GAMEDATA API in SDK 280.
    #include <sysutil/sysutil_gamedata.h>
#endif


namespace EA
{
    namespace IO
    {
        #if (CELL_SDK_VERSION < 0x280000) // Sony removed support for the GAMEDATA API in SDK 280.

        /// PS3GameData
        ///
        /// This is a utility class which does a lite wrapping of the PS3 "game data"
        /// functionality. This class is useful for basic application hard drive access
        /// and for demonstration purposes. A shipping application with advanced game
        /// data usage may want to implement a custom version of this kind of functionality.
        ///
        /// Example usage:
        ///     PS3GameData ps3GameData("TEST-1234", "Super Baseball");
        ///     if(ps3GameData.GetGameDataDirectory())
        ///         printf("Game data dir: %s\n", ps3GameData.GetResults().gameDataPath);
        ///
        class PS3GameData
        {
        public:
            // pTitleId must consist of exactly 9 chars of [A-Z][0-9][-={}<> (plus some other symbol chars)]
            // Sony states that pTitleId should be a "character string of the title product code with the hyphen removed."
            // pTitleName is an arbitrary descriptive string of less than CELL_GAMEDATA_SYSP_TITLE_SIZE bytes.
            // pTitleId is used as the name of the directory that will be returned. 
            // Thus "title id" and "directory name" are synonymous.
            PS3GameData(const char* pTitleId = NULL, const char* pTitleName = NULL);

            /// Constructor based on CellGameDataSystemFileParam.
            PS3GameData(CellGameDataSystemFileParam* pParam);

            /// Returns the path to the PS3-supplied game data directory for the application.
            /// This function is synchronous. When it returns non-NULL, the detailed  
            /// results of the request can be retrieved via the GetResults function.
            /// The first time this is called, it uses the PS3 GameData API to 
            /// acquire a directory path.
            /// The returned directory path has a trailing path separator.
            const char* GetGameDataDirectory(const char* pTitleId = NULL, const char* pTitleName = NULL);

            /// This function returns the lower level CellGameDataStatGet info 
            /// associated with the game data directory system request that we make.
            /// The data returned here is valid only if GetGameDataDirectory was
            /// called and succeeded.
            CellGameDataStatGet& GetResults()
                { return mStatus; }

        protected:
            static PS3GameData* spThis;
            static void StaticGameDataCallback(CellGameDataCBResult* pResult, CellGameDataStatGet* pGetStatus, CellGameDataStatSet* pSetStatus);

        protected:
            sys_memory_container_t      mMemoryContainerId;
            CellGameDataStatGet         mStatus;
            CellGameDataSystemFileParam mParameters;
        };

        #endif // (CELL_SDK_VERSION < 0x280000)

    } // namespace IO

} // namespace EA


#endif // Header include guard












