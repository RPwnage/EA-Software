///////////////////////////////////////////////////////////////////////////////
// IGOSharedStructs.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOSHAREDSTRUCTS_H
#define IGOSHAREDSTRUCTS_H


#ifdef ORIGIN_PC
#include "EABase\eabase.h"
#endif
namespace OriginIGO {

    // - this is a temporary storage for IGO application data, that is usually only stored in the Origin.exe process, but
    //   if Origin.exe is closed and then restarted, the game process reconnects and then we have to use this information from the game process,
    //   because Origin.exe does not remember it launched a game process, so all started games would loose their default URL, content Id and game title
    class GameLaunchInfo
    {
    public:
        GameLaunchInfo();
        
        void setIsValid(bool flag);
        bool isValid() const;

        void setProductId(const char16_t* productId, size_t length);
        void setAchievementSetId(const char16_t* achievementSetId, size_t length);
        void setExecutablePath(const char16_t* executablePath, size_t length);
        void setTitle(const char16_t* title, size_t length);
        void setMasterTitle(const char16_t* title, size_t length);
        void setMasterTitleOverride(const char16_t* title, size_t length);
        void setDefaultURL(const char16_t* url, size_t length);
        
        void setIsTrial(bool isTrial);
        void setTrialTerminated(bool terminated);
        void setTimeRemaining(int64_t timeRemaining);
        void setBroadcastStats(bool isBroadcasting, uint64_t streamId, const char16_t* channel, uint32_t minViewers, uint32_t maxViewers);
        void resetBroadcastStats();
        
        const char16_t* productId() const;
        const char16_t* achievementSetId() const;
        const char16_t* executablePath() const;
        const char16_t* title() const;
        const char16_t* masterTitle() const;
        const char16_t* masterTitleOverride() const;
        const char16_t* defaultURL() const;

        bool isTrial() const;
        bool trialTerminated() const;
        int64_t trialTimeRemaining() const;
        
        bool broadcastStatsChanged() const;
        void broadcastStats(bool& isBroadcasting, uint64_t& streamId, const char16_t** channel, uint32_t &minViewers, uint32_t &maxViewers);
        
        size_t gliStrlen(const char16_t* src) const;
        void gliMemcpy(char16_t* dest, size_t maxLen, const char16_t* src, size_t length);
        
    private:
        struct Data
        {
            // Note: I'm not using eastl::string16 here because we may be setting the data before the game
            // finished its setup, which may involve using different memory allocators... far fetch, but better
            // be safe
            char16_t productId[64];
            char16_t achievementSetId[128];
            char16_t executablePath[512];
            char16_t defaultUrl[512];
            char16_t title[128];
            char16_t masterTitle[128];
            char16_t masterTitleOverride[128];

            // share broadcasting data so we can use telemetry on it in the Origin client
            uint64_t broadcastingStreamId;
            char16_t broadcastingChannel[256];
            uint32_t minBroadcastingViewers;
            uint32_t maxBroadcastingViewers;
            bool isBroadcasting;
            bool broadcastingDataChanged;
            
            //trial info so we can stop the trial from the IGO dll
            int64_t trialTimeRemaining;
            bool forceKillAtOwnershipExpiry;
            bool trialTerminated;
        };
        
        Data mData;
        bool mIsValid;   // true once all the initial data was sent from the Origin client
    };

    GameLaunchInfo* gameInfo();

    
    typedef struct _igoConfigurationFlags{
    #ifdef _CURRENTLY_DISABLED_DLL_LOADING_PREVENTION_CODE_
        bool gNo3rdPartyDetection;
    #endif
        bool gEnabled;
        bool gEnabledLogging;
        bool gEnabledDebugMenu;
        bool gEnabledStressTest;
        int gCurrentGameCount;
    }IGOConfigurationFlags;

    IGOConfigurationFlags &getIGOConfigurationFlags();  

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // Supported renderers
    enum RendererType
    {
        RendererNone,
        RendererDX8,
        RendererDX9,
        RendererDX10,
        RendererDX11,
        RendererDX12,
        RendererMantle,
        RendererOGL
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    // SDK signal detection 
    #define OIG_SDK_SIGNAL_NAME L"Local\\IGO_LOADED_BY_THE_SDK"

    // Telemetry API
    static const uint32_t TelemetryVersion = 1;

    #define OIG_TELEMETRY_PRODUCTID_ENV_NAME L"OIG_TELEMETRY_PRODID"
    #define OIG_TELEMETRY_TIMESTAMP_ENV_NAME L"OIG_TELEMETRY_TS"

    #define OIG_TELEMETRY_PRODUCTID_ENV_NAME_A "OIG_TELEMETRY_PRODID"
    #define OIG_TELEMETRY_TIMESTAMP_ENV_NAME_A "OIG_TELEMETRY_TS"

    #define OIG_TELEMETRY_STRUCTURE_SIZE_MISMATCH "Memory structure size mismatch!"

    enum TelemetryFcn
    {
        TelemetryFcn_IGO_HOOKING_BEGIN = 0,
        TelemetryFcn_IGO_HOOKING_FAIL,
        TelemetryFcn_IGO_HOOKING_INFO,
        TelemetryFcn_IGO_HOOKING_END,

        TelemetryFcn_COUNT
    };

    enum TelemetryContext
    {
        TelemetryContext_GENERIC = 0,
        TelemetryContext_THIRD_PARTY,
        TelemetryContext_MHOOK,
        TelemetryContext_RESOURCE,
        TelemetryContext_HARDWARE,

        TelemetryContext_COUNT
    };

    enum TelemetryRenderer // Matching IGOApplication::RendererType
    {
        TelemetryRenderer_UNKNOWN = 0,
        TelemetryRenderer_DX7,
        TelemetryRenderer_DX8,
        TelemetryRenderer_DX9,
        TelemetryRenderer_DX10,
        TelemetryRenderer_DX11,
		TelemetryRenderer_DX12,
        TelemetryRenderer_MANTLE,
        TelemetryRenderer_OPENGL,

        TelemetryRenderer_COUNT
    };


#pragma pack(push)
#pragma pack(1)
    union TelemetryInfo
    {
        struct // for HOOKING_FAIL, HOOKING_INFO
        {
            char renderer[8];
#ifdef ORIGIN_MAC
            char message[128]; // Limit size for now so that we don't have to use the shared memory to communicate
#else
            char message[256];
#endif
        } msg;

        struct // for HOOKING_START
        {
        } beginStats;

        struct // for HOOKING_END
        {
        } endStats;
    };

    struct TelemetryMsg
    {
        uint32_t version;
        uint32_t pid;
        int64_t timestamp;

#ifdef ORIGIN_PC
        char16_t productId[64];
#else
        char productId[32]; // Limit size for now so that we don't have to use the shared memory to communicate
#endif

        uint32_t fcn; // TelemtryFcn
        char context[16];

        TelemetryInfo info;
    };
#pragma pack(pop)
    
    #define CALL_ONCE_ONLY(fcnCall) \
    { \
        static bool calledOnce = false; \
        if (!calledOnce) \
        { \
            calledOnce = true; \
            fcnCall; \
        } \
    }
}
#endif