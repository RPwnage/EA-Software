#pragma once

#include <eastl/map.h>
#include <eastl/string.h>
#include <eastl/vector.h>

#include <Engine/Core/StreamInstall/StreamInstall.h>
#include <Engine/Core/Message/CoreMessages.h>

#include <QString>

namespace fb
{
    void MallocScope();

    typedef eastl::map< unsigned int, eastl::vector<eastl::string> > ChunkMapping;
    typedef eastl::map< eastl::string, eastl::vector<unsigned int> > GroupMapping;

    struct ManifestContent
    {
        ManifestContent() : m_intialChunkCount(0) { }

        eastl::vector<uint> m_chunkIds;
        GroupMapping m_groups;
        ChunkMapping m_chunks;
        int m_startChunkId;
        int m_intialChunkCount;
    };


    class StreamInstallCommon
    {
    public:
        StreamInstallCommon();

        bool manifestExist();
        const char * getManifestFilename();

        bool superbundleExists(const char* superbundle, MediaHint hint);
	    void mountSuperbundle(const char* superbundle, MediaHint hint, bool optional);
	    void unmountSuperbundle(const char* superbundle);
	    bool isInstalling();
	    bool isSuperbundleInstalled(const char* fileName);
        bool isGroupInstalled(const char* groupName);

    protected:
        void parseManifest(ManifestContent& manifest);
        void setupData(ManifestContent& manifest);
        void cleanupData();

        void chunkInstalled(uint chunkId);
        void groupInstalled(const char * groupName);
    private:
        void initManifestPath();

        QString _manifestPath;
    };
}//fb