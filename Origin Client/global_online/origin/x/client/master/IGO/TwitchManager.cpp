///////////////////////////////////////////////////////////////////////////////
// TwitchManager.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "twitchmanager.h"
#include "twitchinterfaces.h"
#include "Origin_h264_encoder.h"
#include "IGO.h"
#include "IGOApplication.h"
#include "IGOLogger.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h> 

#include "IGOSharedStructs.h"
#define USE_ORIGIN_ENCODER


namespace OriginIGO {

	class OriginH264Plugin: public ITTVPluginVideoEncoder
	{
	public:
		OriginH264Plugin();

		TTV_ErrorCode Start(const TTV_VideoParams* videoParams) override;
		TTV_ErrorCode GetSpsPps(ITTVBuffer* outSps, ITTVBuffer* outPps) override;
		TTV_ErrorCode EncodeFrame(const EncodeInput& input, EncodeOutput& output) override;

		TTV_YUVFormat GetRequiredYUVFormat() const override { return TTV_YUV_NONE; }
	private:
		uint mOutputWidth;
		uint mOutputHeight;
		int  mFramesDelayed;
        uint32_t mStartTimestamp;
        uint32_t mNumFramesEncoded;

	};

	static OriginH264Plugin sOriginH264EncoderPlugin;

    extern HINSTANCE gInstDLL;
    extern HWND gHwnd;

    TTV_ErrorCode TwitchManager::mRequestAuthTokenStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mBufferUnlockStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mLoginStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mGetChannelStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mIngestServersStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mGetStreamInfoStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mArchivingStateStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mSetStreamInfoStatus = TTV_WRN_NOMOREDATA;

    TTV_ErrorCode TwitchManager::mTwitchAPIStartStatus = TTV_WRN_NOMOREDATA;
    TTV_ErrorCode TwitchManager::mTwitchAPIStopStatus = TTV_WRN_NOMOREDATA;

    TTV_IngestList TwitchManager::mIngestList = {0};
    TTV_IngestServer TwitchManager::mServer = {0};
    TTV_StreamInfo TwitchManager::mStreamInfo = {0};
    TTV_ChannelInfo TwitchManager::mChannelInfo = {0};
    TTV_ArchivingState TwitchManager::mArchivingState = {0};
    
    HMODULE TwitchManager::mTwitchDll = NULL;
    TTV_VideoParams TwitchManager::mVideoParams = {0};
    TTV_AudioParams TwitchManager::mAudioParams = {0};
    uint32_t TwitchManager::mInputWidth = 0;
    uint32_t TwitchManager::mInputHeight = 0;
    bool TwitchManager::isBroadCastingInitiated = false;
    bool TwitchManager::isStreamReady = false;
    bool TwitchManager::isBroadCastingRunning = false;
    wchar_t TwitchManager::mTwitchDllFolder[MAX_PATH] = {0};
    bool TwitchManager::mInitialized = false;
    bool TwitchManager::mSuppressFollowupErrors = false;
    TTV_AuthToken TwitchManager::mAuthToken = {0};
    int TwitchManager::mStreamWidth = 0;
    int TwitchManager::mStreamHeight = 0;
    TwitchManager::tPixelBufferList TwitchManager::mPixelBufferList;
    EA::Thread::Futex TwitchManager::mPixelBufferListMutex;
    EA::Thread::Futex TwitchManager::mTwitchAPILock;
    int TwitchManager::mUsedPixelBuffers = 0;
    volatile bool TwitchManager::mIsShuttingDown = false;
    float TwitchManager::mMicVolume = 0.0f;
    float TwitchManager::mGameVolume = 0.0f;
    bool TwitchManager::mMicMuted = false;
    bool TwitchManager::mGameMuted = false;
    bool TwitchManager::mCaptureMicEnabled = false;
	bool TwitchManager::mUseOptEncoder = false;
    TwitchManager::BroadcastThread TwitchManager::mBroadcastStallCheck;
    int TwitchManager::mLastErrorCode = 0;
    TwitchManager::ERROR_CATEGORY TwitchManager::mLastErrorCategory = ERROR_CATEGORY_NONE;
	uint32_t TwitchManager::mFrameCount = 0;	// 
	bool TwitchManager::mUseOriginEncoder = false;
	int TwitchManager::mRendererType = 0;

    TwitchManager::TwitchManager()
    {
    }

    TwitchManager::~TwitchManager()
    {
        TTV_Stop();
        TTV_Shutdown();
    }

    HMODULE TwitchManager::getTwitchDll()
    {
        static HMODULE hTwitch = NULL;

        if (hTwitch)
            return hTwitch;

        const WCHAR *twitchDLL = 
#ifdef _WIN64
    #ifdef _DEBUG
        L"twitchsdk_64_debug.dll";
    #else
        L"twitchsdk_64_release.dll";
    #endif
#else
    #ifdef _DEBUG
        L"twitchsdk_32_debug.dll";
    #else
        L"twitchsdk_32_release.dll";
    #endif
#endif
        WCHAR currentIGOFolder[_MAX_PATH] = {0};
        WCHAR origDllPath[32768] = {0};

        // get the IGO runtime folder
        ::GetModuleFileName(gInstDLL, currentIGOFolder, _MAX_PATH);
        {
            // remove dll name from folder
            wchar_t* strLastSlash = wcsrchr(currentIGOFolder, L'\\');
            if (strLastSlash)
                strLastSlash[1]=0;
#ifdef _WIN64   // twitch for x64 is under a x64 folder, because of it's non x64 dll names
            wcscat_s(currentIGOFolder, _MAX_PATH, L"x64\\");
#endif
            GetDllDirectoryW(_countof(origDllPath), origDllPath);
            SetDllDirectoryW(currentIGOFolder);
            wcscpy_s(mTwitchDllFolder, _countof(mTwitchDllFolder), currentIGOFolder);

            wcscat_s(currentIGOFolder, _MAX_PATH, twitchDLL);
        }

        hTwitch = GetModuleHandle(twitchDLL);
        if (!hTwitch)
        {
            hTwitch = LoadLibrary(currentIGOFolder);
        }
        SetDllDirectoryW(origDllPath);
    
        return hTwitch;
    }

    //////////////////////////////////////////////////////////
    /// https://github.com/justintv/Twitch-API/wiki/Streams-Resource
    /// JSON implementation like other tasks in the twitch SDK
    /// TBD: how to get the current channel id
    /// \brief Get broadcast information
    int TwitchManager::TTV_GetNumberOfViewers(int channel_id)
    {
        return 0;
    }
 
     
    //////////////////////////////////////////////////////////
    /// \brief Internal twitch status
    /// \return Returns the twich SDK DLL state
    bool TwitchManager::IsTTVDLLReady(bool loadIfNotReady /* = true */)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (loadIfNotReady)
            mTwitchDll = getTwitchDll();
        
        return mTwitchDll != NULL;
    }
 
    //////////////////////////////////////////////////////////
    /// \brief Queries the twitch SDK for it's internal state
    /// \return Returns the twich SDK state
    TTV_ErrorCode TwitchManager::QueryTTVStatus(TWITCH_STATUS_QUERY query)
    {
        switch(query)
        {

        case TTV_REQUEST_AUTH_TOKEN_STATUS:
            return mRequestAuthTokenStatus;
            break;
        case TTV_BUFFER_UNLOCK_STATUS:
            return mBufferUnlockStatus;
            break;
        case TTV_LOGIN_STATUS:
            return mLoginStatus;
            break;
        case TTV_GET_INGEST_SERVERS_STATUS:
            return mIngestServersStatus;
            break;
        case TTV_GET_STREAM_INFO_STATUS:
            return mGetStreamInfoStatus;
            break;
        case TTV_SET_STREAM_INFO_STATUS:
            return mSetStreamInfoStatus;
            break;
        case TTV_GET_ARCHIVING_STATE_STATUS:
            return mArchivingStateStatus;
            break;
        case TTV_GETCHANNEL_ONLY_STATUS:
            return mGetChannelStatus;
            break;
        case TTV_TWITCHAPI_START_STATUS:
            return mTwitchAPIStartStatus;
            break;            

        }
        return TTV_EC_NOT_INITIALIZED;
    }
 
    //////////////////////////////////////////////////////////
    /// \brief TTV_SetStreamInfo - Send game-related information to Twitch
    /// \param[in] gameInfo - The game-related information to send.     
    TTV_ErrorCode TwitchManager::TTV_SetStreamInfo(const TTV_StreamInfoForSetting* streamInfo)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady() && mInitialized)
        {
            // no auth token, no authentication, no broadcast
            if(mAuthToken.data[0] == NULL)
                return TTV_EC_AUTHENTICATION;

            mSetStreamInfoStatus = TTV_EC_REQUEST_PENDING;
            
            typedef TTV_ErrorCode (*TTV_SetStreamInfoFunc)(const TTV_AuthToken* authToken, const char* channel, const TTV_StreamInfoForSetting* streamInfoToSet, TTV_TaskCallback callback, void* userData);
            TTV_SetStreamInfoFunc TTV_SetStreamInfoCall = (TTV_SetStreamInfoFunc)GetProcAddress(mTwitchDll, "TTV_SetStreamInfo");
            if (TTV_SetStreamInfoCall)
            {
                return TTV_SetStreamInfoCall(&mAuthToken, mChannelInfo.name, streamInfo, setStreamInfoCallback, NULL);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }



	int DetectNumOfCores()
	{
		uint64_t process_affmask = 0, system_affmask = 0;
		GetProcessAffinityMask(GetCurrentProcess(), (PDWORD_PTR) &process_affmask, (PDWORD_PTR) &system_affmask);
		int num_of_cores = 0;

		for (int i = 0; i < 64; i++)
		{
			if (process_affmask & ((uint64_t) 1L << i))
			{
				num_of_cores++;
			}
		}

		return num_of_cores;
	}
	
	bool HasSSSE3Support()
	{
		int info[4];
		__cpuid(info, 0);
		int nIds = info[0];
		bool HW_SSSE3 = false;

		//  Detect Features
		if (nIds >= 0x00000001)
		{
			__cpuid(info,0x00000001);
			HW_SSSE3  = (info[2] & ((int)1 <<  9)) != 0;
		}

		return HW_SSSE3;
	}

	void TwitchManager::TTV_SetRendererType(int render_type)
	{
		mRendererType = render_type;

		int num_of_cpus = DetectNumOfCores();
		bool hasSSS3support = HasSSSE3Support();

		IGOLogWarn("RenderType %d  Num Of CPUs:%d  Has SSSE3 Support: %d\n", render_type, num_of_cpus, hasSSS3support);
		if ((num_of_cpus >= 4) && ((render_type == RendererDX11) || (render_type == RendererDX10) || (render_type == RendererMantle)))
			mUseOriginEncoder = true;
		else
			mUseOriginEncoder = false;

		//send a "broadcast opt encoder available" message to set the broadcast dialog check box to enabled/disabled
		IGO_ASSERT(IGOApplication::instance()!=NULL);
		SAFE_CALL(IGOApplication::instance(), &IGOApplication::sendBroadcastOptEncoderAvailable, mUseOriginEncoder);
	}

	bool TwitchManager::TTV_IsOriginEncoder()
	{
		return mUseOriginEncoder && mUseOptEncoder;
	}

    //////////////////////////////////////////////////////////
    /// \briefTTV_Init - Initialize the Twitch Broadcasting SDK
    /// \param[in] memCallbacks - Memory allocation/deallocation callback functions provided by the client. If NULL, malloc/free will be used
    /// \param[in] caCertFile - Full path of the CA Cert bundle file (Strongly encourage using the bundle file provided with the SDK)
    /// \param[in] dllPath - [Optional] Windows Only - Path to DLL's to load if no in exe folder (e.g. Intel DLL)
    /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_Init()
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;
        if (IsTTVDLLReady())
        {

            if (mInitialized)
                return TTV_EC_SUCCESS;
                       
            typedef TTV_ErrorCode (*TTV_InitFunc)(const TTV_MemCallbacks* memCallbacks, 
												  const char* clientID,
                                                  const wchar_t* dllPath);

		    TTV_InitFunc TTV_InitCall = (TTV_InitFunc)GetProcAddress(mTwitchDll, "TTV_Init");
		    if (TTV_InitCall)
		    {
				const char* kClientId = "coi1gbc524xzypbgnq1i9bwpj";

				result = TTV_InitCall(NULL, kClientId, mTwitchDllFolder);
                if (TTV_SUCCEEDED(result))
                    mInitialized = true;
            }
        }

    #ifdef _DEBUG
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_SetTraceLevelFunc)(TTV_MessageLevel traceLevel);

            TTV_SetTraceLevelFunc TTV_SetTraceLevelCall = (TTV_SetTraceLevelFunc)GetProcAddress(mTwitchDll, "TTV_SetTraceLevel");
            if (TTV_SetTraceLevelCall)
            {
                TTV_SetTraceLevelCall(TTV_ML_DEBUG);
            }

            typedef TTV_ErrorCode (*TTV_SetTraceOutputFunc)(const wchar_t* outputFileName);

            TTV_SetTraceOutputFunc TTV_SetTraceOutputCall = (TTV_SetTraceOutputFunc)GetProcAddress(mTwitchDll, "TTV_SetTraceOutput");
            if (TTV_SetTraceOutputCall)
            {
                TTV_SetTraceOutputCall(L"twitch.log");
            }
        }
    #endif

        return result; 

    }
 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_GetDefaultParams - Fill in the video parameters with default settings based on supplied resolution
    /// \param[in/out] vidParams - The video params to be filled in with default settings
    /// NOTE: The width and height members of the vidParams MUST be set by the caller
    /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_GetDefaultParams(TTV_VideoParams* vidParams)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_GetDefaultParamsFunc)(TTV_VideoParams* vidParams);

            TTV_GetDefaultParamsFunc TTV_GetDefaultParamsCall = (TTV_GetDefaultParamsFunc)GetProcAddress(mTwitchDll, "TTV_GetDefaultParams");
            if (TTV_GetDefaultParamsCall)
            {
                return TTV_GetDefaultParamsCall(vidParams);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }
 
 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_PollTasks - Polls all currently executing async tasks and if they've finished calls their callbacks
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_PollTasks()
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_PollTasksFunc)();

            TTV_PollTasksFunc TTV_PollTasksCall = (TTV_PollTasksFunc)GetProcAddress(mTwitchDll, "TTV_PollTasks");
            if (TTV_PollTasksCall)
            {
                bool startState = isBroadCastingInitiated;
                result = TTV_PollTasksCall();
                bool endState = isBroadCastingInitiated;

                if (TTV_FAILED(result) || startState!=endState /*if an error was set by a callback, the state will change*/ )
                {
                    TTV_Stop();
                    TTV_Shutdown();
                }
            }
        }

        return result;
    }
 
 
     
    //////////////////////////////////////////////////////////
    /// \briefTTV_RequestAuthToken - Request an authentication key based on the provided username and password.
    /// \param[in] authParams - Authentication parameters
    /// \param[in] callback - The callback function to be called when the request is completed
    /// \param[in] userData - Optional pointer to be passed through to the callback function
    /// \param[in/out] authToken - The authentication token to be written to
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_RequestAuthToken(const TTV_AuthParams* authParams,
                                                    void* userData,
                                                    TTV_AuthToken* authToken)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_RequestAuthTokenFunc)(const TTV_AuthParams* authParams,
													uint32_t flags,	// new with 6.04
                                                    TTV_TaskCallback callback,
                                                    void* userData,
                                                    TTV_AuthToken* authToken);

            TTV_RequestAuthTokenFunc TTV_RequestAuthTokenCall = (TTV_RequestAuthTokenFunc)GetProcAddress(mTwitchDll, "TTV_RequestAuthToken");
            if (TTV_RequestAuthTokenCall)
            {
				return TTV_RequestAuthTokenCall(authParams, (TTV_RequestAuthToken_Broadcast	| TTV_RequestAuthToken_Chat), requestAuthTokenCallback, userData, authToken);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }
 
 
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_GetIngestServers(const TTV_AuthToken* authToken,
                                                              void* userData)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
		IGO_TRACEF("TTV_GetIngestServers: authtoken: %s\n", authToken->data);

        if (IsTTVDLLReady() && mInitialized)
        {
            // no auth token, no authentication, no broadcast
            if(authToken == NULL || authToken->data[0] == NULL)
                return TTV_EC_AUTHENTICATION;

            typedef TTV_ErrorCode (*TTV_GetIngestServersFunc)(const TTV_AuthToken* authToken,
                                                  TTV_TaskCallback callback,
                                                  void* userData,
                                                  TTV_IngestList* ingestList);

            mIngestServersStatus = TTV_EC_REQUEST_PENDING;
            TTV_GetIngestServersFunc TTV_GetIngestServersCall = (TTV_GetIngestServersFunc)GetProcAddress(mTwitchDll, "TTV_GetIngestServers");
            if (TTV_GetIngestServersCall)
            {
                memset(&mIngestList, 0, sizeof(mIngestList));

				IGO_TRACEF("calling TTV_GetIngestServers");

                return TTV_GetIngestServersCall(authToken, getIngestServersCallback, userData, &mIngestList);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }


    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_GetArchivingState(const TTV_AuthToken* authToken,
                                                                   void* userData)
    {

        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady() && mInitialized)
        {
            // no auth token, no authentication, no broadcast
            if(authToken == NULL || authToken->data[0] == NULL)
                return TTV_EC_AUTHENTICATION;

            typedef TTV_ErrorCode (*TTV_GetArchivingStateFunc)(const TTV_AuthToken* authToken,
                                                               TTV_TaskCallback callback,
                                                               void* userData,
                                                               TTV_ArchivingState* archivingState);

            mArchivingStateStatus = TTV_EC_REQUEST_PENDING;
            TTV_GetArchivingStateFunc TTV_GetArchivingStateCall = (TTV_GetArchivingStateFunc)GetProcAddress(mTwitchDll, "TTV_GetArchivingState");
            if (TTV_GetArchivingStateCall)
            {
                memset(&mArchivingState, 0, sizeof(mArchivingState));
                mArchivingState.size = sizeof(mArchivingState);
                return TTV_GetArchivingStateCall(authToken, getArchivingStateCallback, userData, &mArchivingState);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }

    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_GetStreamInfo(const TTV_AuthToken* authToken,
                                                              void* userData)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady() && mInitialized)
        {
            // no auth token, no authentication, no broadcast
            if(authToken == NULL || authToken->data[0] == NULL)
                return TTV_EC_AUTHENTICATION;

            typedef TTV_ErrorCode (*TTV_GetStreamInfoFunc)(const TTV_AuthToken* authToken,
                                                          TTV_TaskCallback callback,
                                                          void* userData,
                                                          const char* channel,
                                                          TTV_StreamInfo* streamInfo);

            mGetStreamInfoStatus = TTV_EC_REQUEST_PENDING;
            TTV_GetStreamInfoFunc TTV_GetStreamInfoCall = (TTV_GetStreamInfoFunc)GetProcAddress(mTwitchDll, "TTV_GetStreamInfo");
            if (TTV_GetStreamInfoCall)
            {
                memset(&mStreamInfo, 0, sizeof(mStreamInfo));
                mStreamInfo.size = sizeof(mStreamInfo);

                return TTV_GetStreamInfoCall(authToken, getStreamInfoCallback, userData, mChannelInfo.name, &mStreamInfo);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }


    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_Login(const TTV_AuthToken* authToken,
                                                void* userData)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;
        if (IsTTVDLLReady() && mInitialized)
        {      
            // no auth token, no authentication, no broadcast
            if(authToken == NULL || authToken->data[0] == NULL)
                return TTV_EC_AUTHENTICATION;

            typedef TTV_ErrorCode (*TTV_LoginFunc)(const TTV_AuthToken* authToken,
                                                TTV_TaskCallback callback,
                                                void* userData,
                                                TTV_ChannelInfo* channelInfo);

            TTV_LoginFunc TTV_LoginCall = (TTV_LoginFunc)GetProcAddress(mTwitchDll, "TTV_Login");
            if (TTV_LoginCall)
            {
                mLoginStatus = TTV_EC_REQUEST_PENDING;
                memset(&mChannelInfo, 0, sizeof(mChannelInfo));
                mChannelInfo.size = sizeof(mChannelInfo);
                result = TTV_LoginCall(authToken, loginCallback, userData, &mChannelInfo);
            }
        }

        return result;
    }
  
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_GetChannelOnly(const TTV_AuthToken* authToken,
                                                void* userData)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;
        if (IsTTVDLLReady() && mInitialized)
        {      
            // no auth token, no authentication, no broadcast
            if(authToken == NULL || authToken->data[0] == NULL)
                return TTV_EC_AUTHENTICATION;

            typedef TTV_ErrorCode (*TTV_LoginFunc)(const TTV_AuthToken* authToken,
                                                TTV_TaskCallback callback,
                                                void* userData,
                                                TTV_ChannelInfo* channelInfo);

            TTV_LoginFunc TTV_LoginCall = (TTV_LoginFunc)GetProcAddress(mTwitchDll, "TTV_Login");
            if (TTV_LoginCall)
            {
                mGetChannelStatus = TTV_EC_REQUEST_PENDING;
                memset(&mChannelInfo, 0, sizeof(mChannelInfo));
                mChannelInfo.size = sizeof(mChannelInfo);
                result = TTV_LoginCall(authToken, loginCallbackChannelOnly, userData, &mChannelInfo);
            }
        }

        return result;
    } 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_Start - Starts streaming
    /// \param[in] videoParams - Output stream video paramaters
    /// \param[in] audioParams - Output stream audio parameters
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_Start(const TTV_VideoParams* videoParams,
                                        const TTV_AudioParams* audioParams)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_StartFunc)(const TTV_VideoParams* videoParams,
                                        const TTV_AudioParams* audioParams,
                                        const TTV_IngestServer* ingestServer,
                                        uint32_t flags,
                                        TTV_TaskCallback callback,
                                        void* userData);

            TTV_StartFunc TTV_StartCall = (TTV_StartFunc)GetProcAddress(mTwitchDll, "TTV_Start");
            if (TTV_StartCall)
            {
                // wait for pending stop call...
                while (mTwitchAPIStopStatus == TTV_EC_REQUEST_PENDING)
                {
                    TTV_PollTasks();
                    Sleep(100);
                }

                mTwitchAPIStopStatus = TTV_WRN_NOMOREDATA;
                mTwitchAPIStartStatus = TTV_EC_REQUEST_PENDING;

				result = TTV_StartCall(videoParams, audioParams, &mServer, 0U, twitchAPIStartStatusCallback, NULL);
				mBroadcastStallCheck.Init();
            }
        }

        return result;
    }

 
 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_SubmitVideoFrame - Submit a video frame to be added to the video stream
    /// \param[in] frameBuffer - Pointer to the frame buffer. This buffer will be considered "locked" until the callback is called.
    /// \param[in] callback - The callback function to be called when buffer is no longer needed
    /// \param[in] userData - Optional pointer to be passed through to the callback function
    /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_SubmitVideoFrame(const uint8_t* frameBuffer,
                                                    void* userData)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);

        if (mFrameCount++ == 0) // throw away first frame as it is usually not valid
        {
            bufferUnlockCallback(frameBuffer, userData);    // unlock the buffer 
            return TTV_EC_SUCCESS;
        }

        // if we submitted a frame, reset our timer... this timer switches a stream to paused, if we do not submit frames within a certain periode, to prevent video stalls
        mBroadcastStallCheck.ResetTimer();

        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;

        if (IsTTVDLLReady() && mInitialized && frameBuffer!=NULL && userData!=NULL)
        {
            typedef TTV_ErrorCode (*TTV_SubmitVideoFrameFunc)(const uint8_t* frameBuffer,
                                                    TTV_BufferUnlockCallback callback,
                                                    void* userData);

            TTV_SubmitVideoFrameFunc TTV_SubmitVideoFrameCall = (TTV_SubmitVideoFrameFunc)GetProcAddress(mTwitchDll, "TTV_SubmitVideoFrame");
            if (TTV_SubmitVideoFrameCall)
            {
				result = TTV_SubmitVideoFrameCall(frameBuffer, bufferUnlockCallback, userData);

                if (TTV_SUCCEEDED(result))
                    mBufferUnlockStatus = TTV_EC_SUCCESS;
                else
                {
                    SetBroadcastError(result);
                    isBroadCastingInitiated = false;
                    SetBroadcastRunning(false);
                }

            }
        }

        return result;
    }
 
 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_SendMetaData - Send some metadata to Twitch
    /// \param[in] metaData - The metadata to send. This must be in valid JSON format
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_SendMetaData(const char* metaData)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);

        return TTV_EC_NOT_INITIALIZED;
    }
 
 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_GetVolume - Get the volume level for the given device
    /// \param[in] device - The device to get the volume for
    /// \param[out] volume - The volume level (0.0 to 1.0)
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_GetVolume(TTV_AudioDeviceType device, float* volume)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_GetVolumeFunc)(TTV_AudioDeviceType device, float* volume);

            TTV_GetVolumeFunc TTV_GetVolumeCall = (TTV_GetVolumeFunc)GetProcAddress(mTwitchDll, "TTV_GetVolume");
            if (TTV_GetVolumeCall)
            {
                return TTV_GetVolumeCall(device, volume);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }
 
 
     
    //////////////////////////////////////////////////////////
    /// \brief TTV_SetVolume - Set the volume level for the given device
    /// \param[in] device - The device to set the volume for
    /// \param[in] volume - The volume level (0.0 to 1.0)
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_SetVolume(TTV_AudioDeviceType device, float volume)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        // currently not used!!!
        // set playback volume    
        // SetWin32Volume(device, volume);

        // set stream volume    
        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_SetVolumeFunc)(TTV_AudioDeviceType device, float volume);

            TTV_SetVolumeFunc TTV_SetVolumeCall = (TTV_SetVolumeFunc)GetProcAddress(mTwitchDll, "TTV_SetVolume");
            if (TTV_SetVolumeCall)
            {
                return TTV_SetVolumeCall(device, volume);
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }

    void TwitchManager::SetBroadcastRunning(bool state)
    {
        if (isBroadCastingRunning != state)
        {
            isBroadCastingRunning = state;
            
            updateGlobalBroadcastStats();

            SAFE_CALL(IGOApplication::instance(), &IGOApplication::sendBroadcastStatus, isBroadCastingRunning, mArchivingState.recordingEnabled, mArchivingState.cureUrl, mChannelInfo.channelUrl);
        }

        if (!isBroadCastingRunning)
            ResetTwitch();
    }

    void TwitchManager::SetBroadcastError(int errorCode, ERROR_CATEGORY errorCategory /* = ERROR_CATEGORY_NONE*/)
    {
        if (!mSuppressFollowupErrors)
        {
            mSuppressFollowupErrors = true; // we only want the first error, not the followup errors
            if (errorCategory == ERROR_CATEGORY_NONE)
            {
                switch (errorCode)
                {
                
                // ebisu_client_broadcast_generic_server
                case TTV_EC_INVALID_CLIENTID:
                case TTV_EC_API_REQUEST_FAILED:
                case TTV_EC_API_REQUEST_TIMEDOUT:
                case TTV_EC_UNKNOWN_ERROR:
                case TTV_EC_NO_INGEST_SERVER_AVAILABLE:
                case TTV_EC_INVALID_INGEST_SERVER:
                case TTV_EC_INVALID_JSON:
                case TTV_EC_WAIT_TIMED_OUT:
                case TTV_EC_RTMP_WRONG_PROTOCOL_IN_URL:
                case TTV_EC_RTMP_UNABLE_TO_SEND_DATA:
                case TTV_EC_RTMP_INVALID_FLV_PACKET:
                case TTV_EC_HTTPREQUEST_ERROR:
                case TTV_EC_INVALID_HTTP_REQUEST_PARAMS:
                case TTV_EC_COINITIALIZE_FAIED:
                case TTV_EC_WEBAPI_RESULT_INVALID_JSON:
                case TTV_EC_WEBAPI_RESULT_NO_AUTHTOKEN:
                case TTV_EC_WEBAPI_RESULT_NO_STREAMKEY:
                case TTV_EC_WEBAPI_RESULT_NO_CHANNELNAME:
                case TTV_EC_WEBAPI_RESULT_NO_INGESTS:
                case TTV_EC_WEBAPI_RESULT_NO_RECORDING_STATUS:
                case TTV_EC_WEBAPI_RESULT_NO_STREAMINFO:
                case TTV_EC_WEBAPI_RESULT_INVALID_VIEWERS:
                case TTV_EC_WEBAPI_RESULT_NO_USERNAME:
                case TTV_EC_WEBAPI_RESULT_NO_USER_DISPLAY_NAME:
                case TTV_EC_INVALID_CALLBACK:
                // socket errors
                case TTV_EC_SOCKET_EINTR:
                case TTV_EC_SOCKET_EBADF:
                case TTV_EC_SOCKET_EACCES:
                case TTV_EC_SOCKET_EFAULT:
                case TTV_EC_SOCKET_EINVAL:
                case TTV_EC_SOCKET_EMFILE:
                case TTV_EC_SOCKET_EWOULDBLOCK:
                case TTV_EC_SOCKET_EINPROGRESS:
                case TTV_EC_SOCKET_EALREADY:
                case TTV_EC_SOCKET_ENOTSOCK:
                case TTV_EC_SOCKET_EDESTADDRREQ:
                case TTV_EC_SOCKET_EMSGSIZE:
                case TTV_EC_SOCKET_EPROTOTYPE:
                case TTV_EC_SOCKET_ENOPROTOOPT:
                case TTV_EC_SOCKET_EPROTONOSUPPORT:
                case TTV_EC_SOCKET_ESOCKTNOSUPPORT:
                case TTV_EC_SOCKET_EOPNOTSUPP:
                case TTV_EC_SOCKET_EPFNOSUPPORT:
                case TTV_EC_SOCKET_EAFNOSUPPORT:
                case TTV_EC_SOCKET_EADDRINUSE:
                case TTV_EC_SOCKET_EADDRNOTAVAIL:
                case TTV_EC_SOCKET_ENETDOWN:
                case TTV_EC_SOCKET_ENETUNREACH:
                case TTV_EC_SOCKET_ENETRESET:
                case TTV_EC_SOCKET_ECONNABORTED:
                case TTV_EC_SOCKET_ECONNRESET:
                case TTV_EC_SOCKET_ENOBUFS:
                case TTV_EC_SOCKET_EISCONN:
                case TTV_EC_SOCKET_ENOTCONN:
                case TTV_EC_SOCKET_ESHUTDOWN:
                case TTV_EC_SOCKET_ETOOMANYREFS:
                case TTV_EC_SOCKET_ETIMEDOUT:
                case TTV_EC_SOCKET_ECONNREFUSED:
                case TTV_EC_SOCKET_ELOOP:
                case TTV_EC_SOCKET_ENAMETOOLONG:
                case TTV_EC_SOCKET_EHOSTDOWN:
                case TTV_EC_SOCKET_EHOSTUNREACH:
                case TTV_EC_SOCKET_ENOTEMPTY:
                case TTV_EC_SOCKET_EPROCLIM:
                case TTV_EC_SOCKET_EUSERS:
                case TTV_EC_SOCKET_EDQUOT:
                case TTV_EC_SOCKET_ESTALE:
                case TTV_EC_SOCKET_EREMOTE:
                case TTV_EC_SOCKET_SYSNOTREADY:
                case TTV_EC_SOCKET_VERNOTSUPPORTED:
                case TTV_EC_SOCKET_NOTINITIALISED:
                case TTV_EC_SOCKET_EDISCON:
                case TTV_EC_SOCKET_ENOMORE:
                case TTV_EC_SOCKET_ECANCELLED:
                case TTV_EC_SOCKET_EINVALIDPROCTABLE:
                case TTV_EC_SOCKET_EINVALIDPROVIDER:
                case TTV_EC_SOCKET_EPROVIDERFAILEDINIT:
                case TTV_EC_SOCKET_SYSCALLFAILURE:
                case TTV_EC_SOCKET_SERVICE_NOT_FOUND:
                case TTV_EC_SOCKET_TYPE_NOT_FOUND:
                case TTV_EC_SOCKET_E_NO_MORE:
                case TTV_EC_SOCKET_E_CANCELLED:
                case TTV_EC_SOCKET_EREFUSED:
                case TTV_EC_SOCKET_HOST_NOT_FOUND:
                case TTV_EC_SOCKET_TRY_AGAIN:
                case TTV_EC_SOCKET_NO_RECOVERY:
                case TTV_EC_SOCKET_NO_DATA:
                case TTV_EC_SOCKET_QOS_RECEIVERS:
                case TTV_EC_SOCKET_QOS_SENDERS:
                case TTV_EC_SOCKET_QOS_NO_SENDERS:
                case TTV_EC_SOCKET_QOS_NO_RECEIVERS:
                case TTV_EC_SOCKET_QOS_REQUEST_CONFIRMED:
                case TTV_EC_SOCKET_QOS_ADMISSION_FAILURE:
                case TTV_EC_SOCKET_QOS_POLICY_FAILURE:
                case TTV_EC_SOCKET_QOS_BAD_STYLE:
                case TTV_EC_SOCKET_QOS_BAD_OBJECT:
                case TTV_EC_SOCKET_QOS_TRAFFIC_CTRL_ERROR:
                case TTV_EC_SOCKET_QOS_GENERIC_ERROR:
                case TTV_EC_SOCKET_QOS_ESERVICETYPE:
                case TTV_EC_SOCKET_QOS_EFLOWSPEC:
                case TTV_EC_SOCKET_QOS_EPROVSPECBUF:
                case TTV_EC_SOCKET_QOS_EFILTERSTYLE:
                case TTV_EC_SOCKET_QOS_EFILTERTYPE:
                case TTV_EC_SOCKET_QOS_EFILTERCOUNT:
                case TTV_EC_SOCKET_QOS_EOBJLENGTH:
                case TTV_EC_SOCKET_QOS_EFLOWCOUNT:
                case TTV_EC_SOCKET_QOS_EUNKOWNPSOBJ:
                case TTV_EC_SOCKET_QOS_EPOLICYOBJ:
                case TTV_EC_SOCKET_QOS_EFLOWDESC:
                case TTV_EC_SOCKET_QOS_EPSFLOWSPEC:
                case TTV_EC_SOCKET_QOS_EPSFILTERSPEC:
                case TTV_EC_SOCKET_QOS_ESDMODEOBJ:
                case TTV_EC_SOCKET_QOS_ESHAPERATEOBJ:
                case TTV_EC_SOCKET_QOS_RESERVED_PETYPE:
                case TTV_EC_SOCKET_SECURE_HOST_NOT_FOUND:
                case TTV_EC_SOCKET_IPSEC_NAME_POLICY_ERROR:
                    errorCategory = ERROR_CATEGORY_SERVER;
                break;

                // ebisu_client_broadcast_token_failure
                case TTV_EC_INVALID_AUTHTOKEN:
                case TTV_EC_AUTHENTICATION:
                case TTV_EC_NEED_TO_LOGIN:
                case TTV_EC_NO_STREAM_KEY:
                    errorCategory = ERROR_CATEGORY_CREDENTIALS;
                break;
    
                // ebisu_client_broadcast_capture_failure
                case TTV_EC_MEMORY:
                case TTV_EC_FAILED_TO_INIT_SPEAKER_CAPTURE:
                case TTV_WRN_FAILED_TO_INIT_MIC_CAPTURE:
                case TTV_EC_UNSUPPORTED_INPUT_FORMAT:
                case TTV_EC_UNSUPPORTED_OUTPUT_FORMAT:
                case TTV_EC_INVALID_RESOLUTION:
                case TTV_EC_INVALID_FPS:
                case TTV_EC_INVALID_BITRATE:
                case TTV_EC_FRAME_QUEUE_FULL:
                case TTV_EC_NO_SPSPPS:
                case TTV_EC_INVALID_VIDEOFRAME:
                case TTV_EC_INTEL_NO_FREE_TASK:
                case TTV_EC_INTEL_NO_FREE_SURFACE:
                case TTV_EC_INTEL_FAILED_VPP_INIT:
                case TTV_EC_INTEL_FAILED_ENCODER_INIT:
                case TTV_EC_INTEL_FAILED_SURFACE_ALLOCATION:
                    errorCategory = ERROR_CATEGORY_CAPTURE_SETTINGS;
                break;

                case TTV_EC_FRAME_QUEUE_TOO_LONG:   // using too much bandwidth
                    errorCategory = ERROR_CATEGORY_SERVER_BANDWIDTH;
                    break;
                
                default:
                    errorCategory = ERROR_CATEGORY_SERVER;    // default category
                    break;
                }
            }

            mLastErrorCategory = errorCategory;
            mLastErrorCode = errorCode;

            SendLastError();
        }
        TTV_Stop();
    }

    void TwitchManager::SendLastError()
    {
        SAFE_CALL(IGOApplication::instance(), &IGOApplication::sendBroadcastError, mLastErrorCategory, mLastErrorCode);

        mLastErrorCategory = ERROR_CATEGORY_NONE;
        mLastErrorCode = 0;
    }

    void TwitchManager::ResetTwitch()
    {
        mRequestAuthTokenStatus = TTV_WRN_NOMOREDATA;
        mBufferUnlockStatus = TTV_WRN_NOMOREDATA;
        mLoginStatus = TTV_WRN_NOMOREDATA;
        mIngestServersStatus = TTV_WRN_NOMOREDATA;
        mGetStreamInfoStatus = TTV_WRN_NOMOREDATA;
        mArchivingStateStatus = TTV_WRN_NOMOREDATA;
        mSetStreamInfoStatus = TTV_WRN_NOMOREDATA;

        // do not reset those flags here !!!
        //mTwitchAPIStartStatus = TTV_WRN_NOMOREDATA;
        //mTwitchAPIStopStatus = TTV_WRN_NOMOREDATA;

        isBroadCastingInitiated = false;
        isStreamReady = false;
        
        FreeAllPixelBuffer();
    }


    bool TwitchManager::TTV_IsScreenSizeEqual(uint32_t width, uint32_t height)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        return (width == mInputWidth && height == mInputHeight);
    }
     
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_FreeIngestList(TTV_IngestList* ingestList)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode error = TTV_EC_NOT_INITIALIZED;

        if (IsTTVDLLReady() && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_FreeIngestListFunc)(TTV_IngestList*);
            TTV_FreeIngestListFunc TTV_FreeIngestListCall = (TTV_FreeIngestListFunc)GetProcAddress(mTwitchDll, "TTV_FreeIngestList");
            if (TTV_FreeIngestListCall)
            {
                error = TTV_FreeIngestListCall(ingestList);
            }
        }

        return error;
    }

    //////////////////////////////////////////////////////////
    /// \brief TTV_Stop - Stops the stream
    /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_Stop()
    {
        mBroadcastStallCheck.Shutdown();

        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode error = TTV_EC_NOT_INITIALIZED;
        SetBroadcastRunning(false);

        if (IsTTVDLLReady(false) && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_StopFunc)(TTV_TaskCallback callback, void* userData);
            TTV_StopFunc TTV_StopCall = (TTV_StopFunc)GetProcAddress(mTwitchDll, "TTV_Stop");
            if (TTV_StopCall)
            {
                // wait for pending TTV_Start start call...
                while (mTwitchAPIStartStatus == TTV_EC_REQUEST_PENDING)
                {
                    TTV_PollTasks();
                    Sleep(100);
                }

                if (mTwitchAPIStartStatus != TTV_WRN_NOMOREDATA)   // TTV_Stop is only needed of we successfully called TTV_Start before!
                {
                    mTwitchAPIStopStatus = TTV_EC_REQUEST_PENDING;
                    mTwitchAPIStartStatus = TTV_WRN_NOMOREDATA;
                    error = TTV_StopCall(twitchAPIStopStatusCallback, NULL);
                    TTV_PollTasks();
                    SetBroadcastRunning(false);
                }
                else
                    mTwitchAPIStopStatus = TTV_WRN_NOMOREDATA;

               
            }
        }

        return error;
    }

    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_Shutdown()
    {
        mBroadcastStallCheck.Shutdown();

        EA::Thread::AutoFutex m(mTwitchAPILock);
        if (IsTTVDLLReady(false) && mInitialized)
        {
            typedef TTV_ErrorCode (*TTV_ShutdownFunc)();
            TTV_ShutdownFunc TTV_ShutdownCall = (TTV_ShutdownFunc)GetProcAddress(mTwitchDll, "TTV_Shutdown");
            if (TTV_ShutdownCall)
            {
                mInitialized = false;
                TTV_ErrorCode error = TTV_ShutdownCall();
                SetBroadcastRunning(false);

                return error;
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }

    TTVSDK_API TTV_ErrorCode TwitchManager::TTV_PauseVideo()
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);

        if (IsTTVDLLReady(false) && mInitialized && isBroadCastingInitiated && isBroadCastingRunning)
        {
            typedef TTV_ErrorCode (*TTV_PauseVideoFunc)();
            TTV_PauseVideoFunc TTV_PauseVideoCall = (TTV_PauseVideoFunc)GetProcAddress(mTwitchDll, "TTV_PauseVideo");
            if (TTV_PauseVideoCall)
            {
                TTV_ErrorCode error = TTV_PauseVideoCall();
                return error;
            }
        }

        return TTV_EC_NOT_INITIALIZED;
    }

    //////////////////////////////////////////////////////////
    /// \brief Internal call to update our global(shared memory) stats for the Origin client
    
    void TwitchManager::updateGlobalBroadcastStats()
    {
        wchar_t channel[256] = {0};
        MultiByteToWideChar(CP_ACP, 0, mChannelInfo.name, _countof(mChannelInfo.name), channel, _countof(channel));

        IGO_ASSERT(channel[0]!=NULL);

        // update our global stats
        gameInfo()->setBroadcastStats(isBroadCastingRunning, mStreamInfo.streamId, channel, mStreamInfo.viewers, mStreamInfo.viewers);
    }

    //////////////////////////////////////////////////////////
    /// \brief Callback function to set internal twitch states

    
    void TwitchManager::twitchAPIStartStatusCallback(TTV_ErrorCode result, void* userData)
    {
        mTwitchAPIStartStatus = result;

        if (TTV_SUCCEEDED(result))
        {
            ApplyVolumeLevels();
            isStreamReady = true;

			const char16_t *masterTitleOverride = gameInfo()->masterTitleOverride();
			eastl::string16 gameTitle;
			gameTitle.clear();

			if (masterTitleOverride)
			{
				gameTitle = masterTitleOverride;
			}

			if (gameTitle.empty())
			{
				const char16_t *masterTitle = gameInfo()->masterTitle();
                if (masterTitle)
                {
                    gameTitle = masterTitle;
                }
            }

            if (!gameTitle.empty())
            {
                TTV_StreamInfoForSetting streamInfo = { 0 };

                WideCharToMultiByte(CP_UTF8, 0, gameTitle.c_str(), -1, streamInfo.gameName, kMaxGameNameLength, NULL, NULL);
                streamInfo.size = sizeof(streamInfo);
                TTV_SetStreamInfo(&streamInfo);
            }

            // do not set streaming to "true" in here, we have not all needed information at this point!
            // the actual stream staus will be changed in getStreamInfoCallback()
        }
        else
        {
            SetBroadcastError(result);
            isBroadCastingInitiated = false;
            SetBroadcastRunning(false);
        }

    }

    void TwitchManager::twitchAPIStopStatusCallback(TTV_ErrorCode result, void* userData)
    {
        mTwitchAPIStopStatus = TTV_EC_SUCCESS;

#ifdef USE_ORIGIN_ENCODER
		if (mUseOriginEncoder && mUseOptEncoder)
			Origin_H264_Encoder_Close();	// must call after TTV_StopCall() otherwise submit frames might be called after this close but before stop call
#endif
    }

    void TwitchManager::bufferUnlockCallback(const uint8_t* buffer, void* userData)
    {
        mBufferUnlockStatus = TTV_WRN_NOMOREDATA;

        MarkUnusedPixelBuffer((tPixelBuffer*)userData);
    }

    void TwitchManager::requestAuthTokenCallback(TTV_ErrorCode result, void* userData)
    {
        mRequestAuthTokenStatus = result;
        IGO_TRACEF("requestAuthTokenCallback: %d", result);
    }

    void TwitchManager::ApplyVolumeLevels()
    {
        if (mGameMuted)
            TTV_SetVolume(TTV_PLAYBACK_DEVICE, 0);
        else
            TTV_SetVolume(TTV_PLAYBACK_DEVICE, mGameVolume);

        if (mMicMuted || !mCaptureMicEnabled)
            TTV_SetVolume(TTV_RECORDER_DEVICE, 0);
        else
            TTV_SetVolume(TTV_RECORDER_DEVICE, mMicVolume);

    }

    void TwitchManager::loginCallbackChannelOnly(TTV_ErrorCode result, void* userData)
    {
        IGO_TRACEF("loginCallback - get channel: %d", result);
        mGetChannelStatus = result;
        if (TTV_SUCCEEDED(result))
        {
            wchar_t *channel = (wchar_t*)userData;
            MultiByteToWideChar(CP_ACP, 0, mChannelInfo.channelUrl, _countof(mChannelInfo.channelUrl), channel, 255);
        }

    }

	void TwitchManager::loginCallback(TTV_ErrorCode result, void* userData)
	{
		IGO_TRACEF("loginCallback: %d", result);
		mLoginStatus = result;
		if (TTV_SUCCEEDED(result) && isBroadCastingInitiated)
		{
			TTV_Start(&mVideoParams, &mAudioParams);
			// handle start result in twitchAPIStartStatusCallback
		}
		else
		{
			if (TTV_FAILED(result))
			{
				SetBroadcastError(result);
				isBroadCastingInitiated = false;
				SetBroadcastRunning(false);
			}
		}
	}

    void TwitchManager::getArchivingStateCallback(TTV_ErrorCode result, void* userData)
    {
        IGO_TRACEF("getArchivingStateCallback: %d", result);
        mArchivingStateStatus = result;

        if (TTV_SUCCEEDED(mArchivingStateStatus))
        {
            //mArchivingState;    // we use it later in SetBroadcastRunning !!!
        }
    }

    void TwitchManager::getStreamInfoCallback(TTV_ErrorCode result, void* userData)
    {
        IGO_TRACEF("getStreamInfoCallback: %d", result);
        mGetStreamInfoStatus = result;
        if (TTV_SUCCEEDED(result) || result == TTV_EC_WEBAPI_RESULT_NO_STREAMINFO)
        {
            if (isStreamReady)
            {
                isStreamReady = false;
                SetBroadcastRunning(true);
            }

            //send a "broadcast started" message and change the streaming status to "true"
            IGO_ASSERT(IGOApplication::instance()!=NULL);
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::sendBroadcastStreamInfo, mStreamInfo.viewers);

            updateGlobalBroadcastStats();
        }
    }

    void TwitchManager::setStreamInfoCallback(TTV_ErrorCode result, void* userData)
    {
        IGO_TRACEF("setStreamInfoCallback: %d", result);
        mSetStreamInfoStatus = result;
        if (TTV_SUCCEEDED(result))
        {
            // nothing for now
        }
    }

    void TwitchManager::getIngestServersCallback(TTV_ErrorCode result, void* userData)
    {
        IGO_TRACEF("getIngestServersCallback: %d (%d)", result, mIngestList.ingestCount);
        mIngestServersStatus = result;
        memset(&mServer, 0, sizeof(mServer));
        if (TTV_SUCCEEDED(result))
        {
            for (unsigned int i = 0; i < mIngestList.ingestCount; ++i)
            {
                if (mIngestList.ingestList[i].defaultServer )
                {
                    // copy the default server
                    memcpy(&mServer, &(mIngestList.ingestList[i]), sizeof(mServer));
                    break;
                }
            }
        }
        else
        {
            if (result == TTV_EC_NO_INGEST_SERVER_AVAILABLE)
                SetBroadcastError(-1, ERROR_CATEGORY_SERVER);   // no ingest servers - heavy load on twitch.tv servers!!!
            else
                SetBroadcastError(result);
        }

        if (mIngestList.ingestCount >0)
        {
            TTV_FreeIngestList(&mIngestList);
        }
    }
     
    bool TwitchManager::IsInitialized()
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        return mInitialized;
    }

    TTV_ErrorCode TwitchManager::IsReadyForStreaming()
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);        
        TTV_ErrorCode result = TTV_EC_NOT_INITIALIZED;
        TTV_ErrorCode internalResult = TTV_EC_NOT_INITIALIZED;
        
        if (TTV_EC_SUCCESS==(internalResult = QueryTTVStatus(TwitchManager::TTV_GET_INGEST_SERVERS_STATUS)))    // we have a server list
        {          
            IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_GET_USER_INFO_STATUS)): %d", internalResult);
            if ((internalResult = QueryTTVStatus(TwitchManager::TTV_LOGIN_STATUS)) == TTV_WRN_NOMOREDATA)   // not yet logged in
            {
                IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_LOGIN_STATUS)): %d", internalResult);
                internalResult = TTV_Login(&mAuthToken, NULL);
                IGO_TRACEF("IsReadyForStreaming (TTV_Login): %d", internalResult);
            }
            else
                if (TTV_EC_SUCCESS==(internalResult = QueryTTVStatus(TwitchManager::TTV_LOGIN_STATUS)))   // logged in
                {
					if (TTV_EC_SUCCESS == (internalResult = TwitchManager::QueryTTVStatus(TwitchManager::TTV_TWITCHAPI_START_STATUS))) // twitch started
					{
						IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_LOGIN_STATUS)): %d", internalResult);

						// query the stream status every 5 seconds
						if ((internalResult = QueryTTVStatus(TwitchManager::TTV_GET_STREAM_INFO_STATUS)) != TTV_EC_REQUEST_PENDING)
						{
							static DWORD mStreamUpdateTime = 0;
							IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_GET_STREAM_INFO_STATUS)): %d", internalResult);
							IGO_TRACEF("IsReadyForStreaming ((GetTickCount() - mStreamUpdateTime) > 5000): %d", ((GetTickCount() - mStreamUpdateTime) > 5000));
							if ((GetTickCount() - mStreamUpdateTime) > 5000)
							{
								TTV_GetStreamInfo(&mAuthToken, NULL);
								mStreamUpdateTime = GetTickCount();
							}
						}
						else
						{
							IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_GET_STREAM_INFO_STATUS)): %d", internalResult);
						}

						result = TTV_EC_SUCCESS;    // everything went fine, now we can return TTV_EC_SUCCESS which starts the stream!!!
					}
                }
                else
                {
                    IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_LOGIN_STATUS)): %d", internalResult);
                }

        }
        else
        {
            if ((internalResult = QueryTTVStatus(TwitchManager::TTV_GET_ARCHIVING_STATE_STATUS)) == TTV_WRN_NOMOREDATA)   // no archiving state yet
            {
                IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_GET_ARCHIVING_STATE_STATUS)): %d", internalResult);
                internalResult = TTV_GetArchivingState(&mAuthToken, NULL);
                IGO_TRACEF("IsReadyForStreaming (TTV_GetArchivingState): %d", internalResult);
            }
                   
            if ((internalResult = QueryTTVStatus(TwitchManager::TTV_GET_INGEST_SERVERS_STATUS)) == TTV_WRN_NOMOREDATA)   // no server list
            {
                IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_GET_INGEST_SERVERS_STATUS)): %d", internalResult);
                internalResult = TTV_GetIngestServers(&mAuthToken, NULL);
                IGO_TRACEF("IsReadyForStreaming (TTV_GetIngestServers): %d", internalResult);
            }
            else
            {
                IGO_TRACEF("IsReadyForStreaming (QueryTTVStatus(TwitchManager::TTV_GET_INGEST_SERVERS_STATUS)): %d", internalResult);
            }
        }

        if (internalResult != TTV_WRN_NOMOREDATA && internalResult != TTV_EC_WEBAPI_RESULT_NO_STREAMINFO && !TTV_SUCCEEDED(internalResult) && internalResult != TTV_EC_REQUEST_PENDING)
        {
            SetBroadcastError(internalResult);
            return internalResult;
        }

        return result;
    }

    TTV_ErrorCode TwitchManager::InitializeForStreaming(int inputWidth, int inputHeight, int &streamWidth, int &streamHeight, int &fps)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        TTV_ErrorCode error = TTV_EC_NOT_INITIALIZED;

		mFrameCount = 0;

        mInputWidth = inputWidth;
        mInputHeight = inputHeight;

        streamWidth = mVideoParams.outputWidth;
        streamHeight = mVideoParams.outputHeight;
        fps = mVideoParams.targetFps;

        // Output streamHeight and streamWidth must be multiple of 32 (required by the Intel encoder)
        if (streamHeight % 32 || streamWidth % 32)
        {
            streamHeight -= (streamHeight % 32);
            streamWidth -= (streamWidth % 32);
        }

        // update our video dimensions
        mVideoParams.outputHeight = mStreamHeight = streamHeight;
        mVideoParams.outputWidth = mStreamWidth = streamWidth;
		mVideoParams.encoderPlugin = NULL;	// default to Intel
#ifdef USE_ORIGIN_ENCODER
		if (mUseOriginEncoder && mUseOptEncoder)
		{
			mVideoParams.encoderPlugin = &sOriginH264EncoderPlugin;
			OriginIGO::IGOLogWarn("Using Origin Encoder");
		}
		else
		{
			OriginIGO::IGOLogWarn("Using Default Intel Encoder");
		}
#endif
        if (IsTTVDLLReady())
        {
			OriginIGO::IGOLogWarn("Calling TTV_Init from InitializeForStreaming");
            error = TTV_Init();
            if (!TTV_SUCCEEDED(error))
            {
                SetBroadcastError(error);
                return error;
            }

            // let twitch calculate some parameters depending on our resolution
            if (TTV_SUCCEEDED(error))
            {
                TTV_EncodingCpuUsage quality = mVideoParams.encodingCpuUsage; // save this value, otherwise it will be set to "lowest"
                error = TTV_GetDefaultParams(&mVideoParams);
                mVideoParams.encodingCpuUsage = quality;
            }
        }

        return error;
    }

    bool TwitchManager::IsBroadCastingInitiated()
    {
        return isBroadCastingInitiated;
    }

    bool TwitchManager::IsBroadCastingRunning()
    {
        return isBroadCastingRunning;
    }

    void TwitchManager::SetBroadcastMicVolumeSetting(int volume)
    {
        mMicVolume = eastl::min(eastl::max(volume, 0), 100) /100.0f;

        ApplyVolumeLevels();
    }

    void TwitchManager::SetBroadcastGameVolumeSetting(int volume)
    {
        mGameVolume = eastl::min(eastl::max(volume, 0), 100) /100.0f;

        ApplyVolumeLevels();
    }

    void TwitchManager::SetBroadcastMicMutedSetting(bool muted)
    {
        mMicMuted = muted;

        ApplyVolumeLevels();
    }

    void TwitchManager::SetBroadcastGameMutedSetting(bool mutedvolume)
    {
        mGameMuted = mutedvolume;
        
        ApplyVolumeLevels();
    }

        
    void TwitchManager::SetBroadcastSettings(int resolution, int framerate, int quality, int bitrate, bool showViewersNum, bool broadcastMic, bool micVolumeMuted, int micVolume, bool gameVolumeMuted, int gameVolume, bool useOptEncoder, eastl::string token)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        IGO_TRACEF("SetBroadcastSettings");
        
        if (kAuthTokenBufferSize < token.length())
        {
            IGO_TRACE("SetBroadcastSettings - token invalid!");
            IGO_ASSERT(0);
            return;
        }

        //copy the token
        memset(&(mAuthToken.data[0]), 0, sizeof(mAuthToken.data));
        memcpy(&(mAuthToken.data[0]), token.c_str(), token.length());


        
        // clear settings
        memset(&(mVideoParams), 0, sizeof(TTV_VideoParams));
        mVideoParams.size = sizeof(TTV_VideoParams);

        mInputWidth = mInputHeight = 0;

        // currently not used with the latst SDK
        bitrate=0;


        if (resolution > 9 || resolution < 0)
        {
            resolution = 9; // we are currently not supporting 1920x1080 due to twitch's decision to cap bandwidth at 3500kbs
        }

        switch(resolution)
        {
        case 0:
            mVideoParams.outputWidth = 320;
            mVideoParams.outputHeight = 240;
            break;
        case 1:
            mVideoParams.outputWidth = 480;
            mVideoParams.outputHeight = 360;
            break;
        case 2:
            mVideoParams.outputWidth = 640;
            mVideoParams.outputHeight = 480;
            break;
        case 3:
            mVideoParams.outputWidth = 1024;
            mVideoParams.outputHeight = 768;
            break;
        case 4:
            mVideoParams.outputWidth = 1280;
            mVideoParams.outputHeight = 960;
            break;
        case 5:
            mVideoParams.outputWidth = 480;
            mVideoParams.outputHeight = 270;
            break;
        case 6:
            mVideoParams.outputWidth = 640;
            mVideoParams.outputHeight = 360;
            break;
        case 7:
            mVideoParams.outputWidth = 800;
            mVideoParams.outputHeight = 450;
            break;
        case 8:
            mVideoParams.outputWidth = 1024;
            mVideoParams.outputHeight = 576;
            break;
        case 9:
            mVideoParams.outputWidth = 1280;
            mVideoParams.outputHeight = 720;
            break;
        }

        if(framerate>5 || framerate<0)
        {
            IGO_ASSERT(0);
            framerate = 5;
        }

        switch(framerate)
        {
        case 0:
            mVideoParams.targetFps = 10;
            break;
        case 1:
            mVideoParams.targetFps = 15;
            break;
        case 2:
            mVideoParams.targetFps = 20;
            break;
        case 3:
            mVideoParams.targetFps = 25;
            break;
        case 4:
            mVideoParams.targetFps = 30;
            break;
        }
       
        mVideoParams.pixelFormat = TTV_PF_BGRA;


        // audio settings
        if(micVolume>100 || micVolume<0)
        {
            IGO_ASSERT(0);
            micVolume = 100;
        }

        if(gameVolume>100 || gameVolume<0)
        {
            IGO_ASSERT(0);
            gameVolume = 100;
        }

        if(quality>2 || quality<0)
        {
            IGO_ASSERT(0);
            quality = 1;    //medium
        }

        mVideoParams.encodingCpuUsage = (TTV_EncodingCpuUsage)quality;

		mUseOptEncoder = useOptEncoder;	// mVideoParams.encoderPlugin set in InitializeForStreaming()

        mCaptureMicEnabled = broadcastMic;

        mMicVolume = eastl::min(eastl::max(micVolume, 0), 100) /100.0f;
        mGameVolume = eastl::min(eastl::max(gameVolume, 0), 100) /100.0f;
        mMicMuted = micVolumeMuted;
        mGameMuted = gameVolumeMuted;

        mAudioParams.size = sizeof(TTV_AudioParams);

        // if we have at least one audio device, activate audio support
        if (IsAudioDevicePresent(TTV_PLAYBACK_DEVICE) || IsAudioDevicePresent(TTV_RECORDER_DEVICE))
		{
            mAudioParams.audioEnabled = true;
			mAudioParams.enableMicCapture = true;
			mAudioParams.enablePlaybackCapture = true;
			mAudioParams.enablePassthroughAudio = false;
		}
        else
            mAudioParams.audioEnabled = false;

    }

    void TwitchManager::SetBroadcastCommand(bool start)
    {
        EA::Thread::AutoFutex m(mTwitchAPILock);
        IGO_TRACEF("SetBroadcastCommand: %d", start);
        SetBroadcastRunning(false);
        if (start)
        {
            TTV_Stop();
			mSuppressFollowupErrors = false;
            isBroadCastingInitiated = true;
        }
        else
        {
            TTV_Stop();
			mSuppressFollowupErrors = false;
            isBroadCastingInitiated = false;
        }
    }

    TwitchManager::tPixelBuffer* TwitchManager::GetUnusedPixelBuffer()
    {
        if (mIsShuttingDown)
            return NULL;

        EA::Thread::AutoFutex m(mPixelBufferListMutex);

        tPixelBufferList::const_iterator iter;
        for (iter = mPixelBufferList.begin(); iter != mPixelBufferList.end(); ++iter)
        {
            // look for old "locked" buffers first
            if ( ((*iter)->height != mStreamHeight || (*iter)->width != mStreamWidth) )
            {
                IGO_TRACE("GetUnusedPixelBuffer - found old buffer, resizing it!");

                if ((*iter)->data != NULL)
                    _aligned_free((*iter)->data);

                (*iter)->data = (unsigned char*)_aligned_malloc(mStreamWidth * mStreamHeight * 4, 16 /*for SSE*/);
                memset((*iter)->data, 0, mStreamWidth * mStreamHeight *4);
                (*iter)->height = (uint16_t)mStreamHeight;
                (*iter)->width = (uint16_t)mStreamWidth;
                
                if (!(*iter)->inUse)
                {
                    mUsedPixelBuffers++;
                    (*iter)->inUse = true;
                }

                return (*iter);
            }
        }

        for (iter = mPixelBufferList.begin(); iter != mPixelBufferList.end(); ++iter)
        {
            // look for unused buffers next
            if ( (*iter)->inUse == false )
            {
                memset((*iter)->data, 0, mStreamWidth * mStreamHeight *4);
                (*iter)->height = (uint16_t)mStreamHeight;
                (*iter)->width = (uint16_t)mStreamWidth;
                mUsedPixelBuffers++;
                (*iter)->inUse = true;
                return (*iter);
            }
        }

        // create a new buffer
        TwitchManager::tPixelBuffer *buffer = new TwitchManager::tPixelBuffer;
        buffer->data = (unsigned char*)_aligned_malloc(mStreamWidth * mStreamHeight * 4, 16 /*for SSE*/);
        memset(buffer->data, 0, mStreamWidth * mStreamHeight *4);
        buffer->height = (uint16_t)mStreamHeight;
        buffer->width = (uint16_t)mStreamWidth;
        mUsedPixelBuffers++;
        buffer->inUse = true;
        mPixelBufferList.push_back(buffer);
        return buffer;
    }

    void TwitchManager::FreeAllPixelBuffer()
    {
        mIsShuttingDown = true;
        EA::Thread::AutoFutex m(mPixelBufferListMutex);
        bool noMoreInUse = true;
        tPixelBufferList::const_iterator iter;
        for (iter = mPixelBufferList.begin(); iter != mPixelBufferList.end(); ++iter)
        {
            // if a buffer is still used, don't release any of them... we release them later
            if ( (*iter)->inUse == true )
            {
                noMoreInUse = false;
                break;
            }
        }

        // only free our buffers if twitch is no longer using them!!!
        if (noMoreInUse)
        {
            for (iter = mPixelBufferList.begin(); iter != mPixelBufferList.end(); ++iter)
            {
                _aligned_free((*iter)->data);
                delete (*iter);
            }
        
            mPixelBufferList.clear();
            mUsedPixelBuffers=0;
        }

        mIsShuttingDown = false;
    }

    void TwitchManager::MarkUnusedPixelBuffer(tPixelBuffer *buffer)
    {
        EA::Thread::AutoFutex m(mPixelBufferListMutex);

        tPixelBufferList::const_iterator iter;
        for (iter = mPixelBufferList.begin(); iter != mPixelBufferList.end(); ++iter)
        {
            if ((*iter) == buffer)
            {
                mUsedPixelBuffers--;
                (*iter)->inUse = false;
                break;
            }
        }
    }

    bool TwitchManager::IsAudioDevicePresent(TTV_AudioDeviceType device)
    {
        bool devicePresent = true;
        HRESULT hrCoInit = CoInitialize(NULL);
        IMMDeviceEnumerator *deviceEnumerator = NULL;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
        IMMDevice *defaultDevice = NULL;
        if (FAILED(hr))
        {
            SAFE_RELEASE(deviceEnumerator);
            if (SUCCEEDED(hrCoInit))
                CoUninitialize();
            return devicePresent;
        }

        ERole role = eConsole;
        EDataFlow flow = (device == TTV_PLAYBACK_DEVICE) ? eRender : eCapture;

        hr = deviceEnumerator->GetDefaultAudioEndpoint(flow, role, &defaultDevice);
        SAFE_RELEASE(deviceEnumerator);
        if (FAILED(hr))
        {
            SAFE_RELEASE(defaultDevice);
            if (SUCCEEDED(hrCoInit))
                CoUninitialize();
            
            // device was not found !!!
            if (hr == E_NOTFOUND)
                devicePresent = false;

            return devicePresent;
        }
        SAFE_RELEASE(defaultDevice);

        if (SUCCEEDED(hrCoInit))
            CoUninitialize();

        return devicePresent;
    }

    void TwitchManager::SetWin32Volume(TTV_AudioDeviceType device, float volume)
    {
        HRESULT hrCoInit = CoInitialize(NULL);
        IMMDeviceEnumerator *deviceEnumerator = NULL;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
        IMMDevice *defaultDevice = NULL;
        if (FAILED(hr))
        {
            SAFE_RELEASE(deviceEnumerator);
            if (SUCCEEDED(hrCoInit))
                CoUninitialize();
            return;
        }
        ERole role = (device == TTV_PLAYBACK_DEVICE) ? eConsole : eCommunications;
        EDataFlow flow = (device == TTV_PLAYBACK_DEVICE) ? eRender : eCapture;

        hr = deviceEnumerator->GetDefaultAudioEndpoint(flow, role, &defaultDevice);
        SAFE_RELEASE(deviceEnumerator);
        if (FAILED(hr))
        {
            SAFE_RELEASE(defaultDevice);
            if (SUCCEEDED(hrCoInit))
                CoUninitialize();
            return;
        }

        IAudioEndpointVolume *endpointVolume = NULL;
        hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
        SAFE_RELEASE(defaultDevice);
        if (FAILED(hr))
        {
            SAFE_RELEASE(endpointVolume);
            if (SUCCEEDED(hrCoInit))
                CoUninitialize();
            return;
        }

        endpointVolume->SetMasterVolumeLevelScalar((float)volume, NULL);
        SAFE_RELEASE(endpointVolume);

        if (SUCCEEDED(hrCoInit))
            CoUninitialize();
    }

    
TwitchManager::BroadcastThread::BroadcastThread() :
    mbThreadShouldQuit(false),
    mPollThreadTimeCheckSeconds(2),
    mThread(),
    mBroadcastThreadLock(),
    mTimeNow(0),
    mTimeToPoll(0)
{
}

TwitchManager::BroadcastThread::~BroadcastThread()
{
    Shutdown();
}


bool TwitchManager::BroadcastThread::Init()
{
    mbThreadShouldQuit = false;
    ResetTimer();
    mThread.Begin(this);
    return true;
}

bool TwitchManager::BroadcastThread::Shutdown()
{       
    if (mbThreadShouldQuit == true)
        return true;

    mbThreadShouldQuit = true;
    if (EA::Thread::Thread::kStatusRunning == mThread.GetStatus())
    {
        while(EA::Thread::Thread::kStatusRunning == mThread.WaitForEnd(EA::Thread::GetThreadTime() + 5000))
        {
            EA::Thread::ThreadSleep(250);
        }
    }
    return true;
}

void TwitchManager::BroadcastThread::ResetTimer()
{
    EA::Thread::AutoFutex m(mBroadcastThreadLock);
    mTimeToPoll = EA::Thread::GetThreadTime() + (mPollThreadTimeCheckSeconds * 1000);
}

intptr_t TwitchManager::BroadcastThread::Run(void* pContext)
{
    while(!mbThreadShouldQuit && TwitchManager::IsInitialized())
    {
        {
            EA::Thread::AutoFutex m(mBroadcastThreadLock);
            mTimeNow = EA::Thread::GetThreadTime();

            if(mTimeNow >= mTimeToPoll && TwitchManager::QueryTTVStatus(TwitchManager::TTV_TWITCHAPI_START_STATUS) != TTV_EC_REQUEST_PENDING)
            {
                TwitchManager::TTV_PauseVideo();

                mTimeToPoll = EA::Thread::GetThreadTime() + (mPollThreadTimeCheckSeconds * 1000);

                if(mTimeToPoll < mTimeNow) // If there was integer wrap around...
                    mTimeNow = mTimeToPoll;
            }
        }
        EA::Thread::ThreadSleep(250);
    }

    return 0;
}

// Generic encoder plugin code for Origin H264 Encoder
//--------------------------------------------------------------------------
OriginH264Plugin::OriginH264Plugin() 
	: mOutputWidth(0)
	, mOutputHeight(0)
	, mFramesDelayed(0)
    , mStartTimestamp(0)
    , mNumFramesEncoded(0)
{

}

//--------------------------------------------------------------------------
TTV_ErrorCode OriginH264Plugin::Start(const TTV_VideoParams* videoParams)
{
	IGO_ASSERT(videoParams);

	mOutputWidth = videoParams->outputWidth;
	mOutputHeight = videoParams->outputHeight;
    mStartTimestamp = 0;
    mNumFramesEncoded = 0;

	// must initialize before TTV_StartCall() as internally it will request the sps and pps nal units which we need to set externally
	OriginEncoder_InputParamsType params;

	params.variable_qp = 0;
	params.subpixel_motion_est = 0;
	params.b_slice_on = 0;

	float bits_per_pixel = 0.1f;

	switch (videoParams->encodingCpuUsage)
	{
	case TTV_ECU_LOW:
		params.filtering_on = 0;	// deblocking filter
		break;
	case TTV_ECU_MEDIUM:
		params.filtering_on = 1;	// deblocking filter
		params.variable_qp = 1;
		bits_per_pixel = 0.107f;	// increase by 7%
		break;
	case TTV_ECU_HIGH:
		params.filtering_on = 1;	// deblocking filter
		params.variable_qp = 1;
		params.subpixel_motion_est = 1;
		bits_per_pixel = 0.115f;	// increase by 15% (over low)
		break;
	}

	float bits_needed = mOutputWidth * mOutputHeight * videoParams->targetFps * bits_per_pixel / 1000.0f;
	if (bits_needed > 3500.0f)
		bits_needed = 3500.0f;

	params.target_bitrate = (int) bits_needed;	// in kbps

	params.width = mOutputWidth;
	params.height = mOutputHeight;
	params.target_frame_rate = videoParams->targetFps;	// in frames per second
	params.max_bitrate = 3500;	// in kbps
	params.idr_frame_interval = videoParams->targetFps * 2;	// frames between idr frames (Twitch requires 2 seconds between idr)
	params.rate_control = 1;	// 0 - off, 1 - on
	params.max_qp_delta = 4;
	IGOLogWarn("OriginH264Plugin::Start - pre-init");
	Origin_H264_Encoder_Init(&params);
	IGOLogWarn("OriginH264Plugin::Start - post-init");

	return TTV_EC_SUCCESS;// : TTV_EC_UNKNOWN_ERROR;
}

//--------------------------------------------------------------------------
TTV_ErrorCode OriginH264Plugin::GetSpsPps(ITTVBuffer* outSps, ITTVBuffer* outPps)
{
	// get and set SPS and PPS NAL units
	int nal_count;
	NAL_Type *nal_from;
	Origin_H264_Encode_Headers(&nal_count, (void **) &nal_from);

	typedef TTV_ErrorCode (*TTV_SetSPSPPS_Func)(int nal_count, void *headerdata);

	outSps->Append(nal_from[0].payload, nal_from[0].payload_size);
	outPps->Append(nal_from[1].payload, nal_from[1].payload_size);

	return TTV_EC_SUCCESS;
}

//--------------------------------------------------------------------------
TTV_ErrorCode OriginH264Plugin::EncodeFrame(const EncodeInput& input, EncodeOutput& output)
{
	Origin_H264_EncoderInputType enc_input;
	Origin_H264_EncoderOutputType enc_output;
	bool key_frame = false;
	int num_nal = 0;
	NAL_Type *nalOut;

    int time_stamp = GetTickCount();
	if ((time_stamp - mStartTimestamp) >= (2000-33))
	{
		key_frame = true;
		mStartTimestamp = time_stamp;
	}

	enc_input.frame_type = ORIGIN_H264_DEFAULT_TYPE;
	if (key_frame)
		enc_input.qp = 24;
	else
		enc_input.qp = 31;
	if(key_frame)
		enc_input.frame_type = ORIGIN_H264_IDR;

    if (mNumFramesEncoded == 0)
        enc_input.timestamp = 0;    // work around for bug with timeStamp initially being garbage
    else
	    enc_input.timestamp = static_cast<uint32_t>(input.timeStamp);
    mNumFramesEncoded++;

	IGOLogWarn("OriginH264Plugin::EncodeFrame - pre-encode");

	if (input.source)
	{
		enc_input.y_data = (uint8_t *) input.source;
		enc_input.u_data = NULL;
		enc_input.v_data = NULL;

		mFramesDelayed = Origin_H264_Encoder_Encode(&enc_input, &enc_output);
		IGOLogWarn("OriginH264Plugin::EncodeFrame - post-encode:  %d ", mFramesDelayed);
	}
	else
	{
		mFramesDelayed = Origin_H264_Encoder_Encode(NULL, &enc_output);
		IGOLogWarn("OriginH264Plugin::EncodeFrame - post-encode2:  %d ", mFramesDelayed);
		if (mFramesDelayed <= 0)
		{
			return TTV_WRN_NOMOREDATA;
		}
	}

	num_nal = enc_output.nal_count;
	nalOut = (NAL_Type *) enc_output.nal_out;

	if (num_nal > 0)
	{
		output.frameTimeStamp = enc_output.timestamp;
		output.isKeyFrame = enc_output.frame_type == ORIGIN_H264_IDR;
		//output.frameData->Resize(nalRet);
		for (int i = 0; i < num_nal; ++i)
		{
			output.frameData->Append(nalOut[i].payload, nalOut[i].payload_size);
		}
		return TTV_EC_SUCCESS;
	}

	return TTV_WRN_NOMOREDATA;
}




}


