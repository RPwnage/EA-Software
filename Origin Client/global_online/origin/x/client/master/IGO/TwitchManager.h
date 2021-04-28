///////////////////////////////////////////////////////////////////////////////
// TwitchManager.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef __TWITCHMANAGER_H__
#define __TWITCHMANAGER_H__

#include "IGO.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector.h"
#include "EASTL/functional.h"
#include "EASTL/string.h"
#include "eathread/eathread_futex.h"
#include "EAThread/eathread_thread.h"

namespace OriginIGO {

    class TwitchManager
    {
    public:

        enum ARTIFICIAL_ERROR{
            BROADCAST_CLOUD_ERROR_NO_TOKEN = -667,    // artifical errors are always in -6XX range!!!
            BROADCAST_NO_CHANNEL_NAME = -666,    // artifical errors are always in -6XX range!!!
            BROADCAST_NO_USER_NAME = -665,    // artifical errors are always in -6XX range!!!
            BROADCAST_REQUEST_TIMEOUT = -664,    // artifical errors are always in -6XX range!!!
        };
        //////////////////////////////////////////////////////////
        /// \brief Error categories to filter our messaging to the user
        enum ERROR_CATEGORY{
            ERROR_CATEGORY_NONE = 0,
            ERROR_CATEGORY_SERVER,
            ERROR_CATEGORY_CAPTURE_SETTINGS,
            ERROR_CATEGORY_CREDENTIALS,
            ERROR_CATEGORY_VIDEO_SETTINGS,
            ERROR_CATEGORY_UNSUPPORTED,
            ERROR_CATEGORY_SERVER_BANDWIDTH,
        };

        //////////////////////////////////////////////////////////
        /// \brief Loads the twitch DLL and binds the functions
        TwitchManager();
 
        //////////////////////////////////////////////////////////
        /// \brief Unloads the twitch DLL
        ~TwitchManager();
        
#ifdef TTVSDK_TWITCH_SDK_H
     

        //////////////////////////////////////////////////////////
        /// \brief Public twitch API used by IGO
        static void SetBroadcastMicVolumeSetting(int volume);
        static void SetBroadcastGameVolumeSetting(int volume);
        static void SetBroadcastMicMutedSetting(bool muted);
        static void SetBroadcastGameMutedSetting(bool mutedvolume);
        static void ApplyVolumeLevels();

        static void SetBroadcastCommand(bool start);
        static bool IsBroadCastingInitiated();
        static bool IsBroadCastingRunning();

        static void SetBroadcastSettings(int resolution, int framerate, int quality, int bitrate, bool showViewersNum, bool broadcastMic, bool micVolumeMuted, int micVolume, bool gameVolumeMuted, int gameVolume, bool useOptEncoder, eastl::string token);



        //////////////////////////////////////////////////////////
        /// \brief Public twitch API used by the renderer
        static bool IsInitialized();
        static TTV_ErrorCode IsReadyForStreaming();
        static TTV_ErrorCode InitializeForStreaming(int inputWidth, int inputHeight, int &streamWidth, int &streamHeight, int &fps);
        static void SetBroadcastError(int errorCode, ERROR_CATEGORY errorCategory = ERROR_CATEGORY_NONE);
        static void SendLastError();

        typedef struct tPixelBuffer{
            unsigned char *data;
            uint16_t width;
            uint16_t height;
            bool inUse;
        }tPixelBuffer;
        typedef eastl::vector<tPixelBuffer*> tPixelBufferList;

        static tPixelBuffer* GetUnusedPixelBuffer();
        static void MarkUnusedPixelBuffer(tPixelBuffer* buffer);
        static void FreeAllPixelBuffer();
        static int GetPixelBufferCount() { return (int)mPixelBufferList.size(); };
        static int GetUsedPixelBufferCount() { return mUsedPixelBuffers; };


        //////////////////////////////////////////////////////////
        /// \brief Internal twitch SDK wrapper API

        //////////////////////////////////////////////////////////
        /// \brief Twitch stats query flags
        enum TWITCH_STATUS_QUERY{
     
            TTV_REQUEST_AUTH_TOKEN_STATUS = 0,
            TTV_BUFFER_UNLOCK_STATUS,
            TTV_LOGIN_STATUS,
            TTV_GET_INGEST_SERVERS_STATUS,
            TTV_GET_ARCHIVING_STATE_STATUS,
            TTV_GET_STREAM_INFO_STATUS,
            TTV_GETCHANNEL_ONLY_STATUS,
            TTV_SET_STREAM_INFO_STATUS,
            TTV_TWITCHAPI_START_STATUS,
     
        };

        //////////////////////////////////////////////////////////
        /// https://github.com/justintv/Twitch-API/wiki/Streams-Resource
        /// JSON implementation like other tasks in the twitch SDK
        /// TBD: how to get the current channel id
        /// \brief Get broadcast information
        static int TTV_GetNumberOfViewers(int channel_id);
 
     
        //////////////////////////////////////////////////////////
        /// \brief Internal twitch status
        /// \param[in] loadIfNotReady - If the dll is not loaded, load it 
        /// \return Returns the twich SDK DLL state
        static bool IsTTVDLLReady(bool loadIfNotReady = true);
 
        //////////////////////////////////////////////////////////
        /// \brief Queries the twitch SDK for it's internal state
        /// \return Returns the twich SDK state
        static TTV_ErrorCode QueryTTVStatus(TWITCH_STATUS_QUERY query);
 

        //////////////////////////////////////////////////////////
        /// \brief TTV_SetGameInfo - Send game-related information to Twitch
        /// \param[in] streamInfo - The stream-related information to send.
        static TTV_ErrorCode TTV_SetStreamInfo(const TTV_StreamInfoForSetting* streamInfo);
        
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_Init - Initialize the Twitch Broadcasting SDK
        /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
        static TTVSDK_API TTV_ErrorCode TTV_Init();
 
        //////////////////////////////////////////////////////////
        /// \brief TTV_GetChannelOnly - Use the given auth token to log into the associated channel. This is used externaly by Origin only!!!
        /// \param[in] authToken - The authentication token previously obtained
        /// \param[in] userData - Optional pointer to be passed through to the callback function
        static TTVSDK_API TTV_ErrorCode TTV_GetChannelOnly(const TTV_AuthToken* authToken,
                                                    void* userData);

     
        //////////////////////////////////////////////////////////
        /// \brief TTV_GetDefaultParams - Fill in the video parameters with default settings based on supplied resolution
        /// \param[in/out] vidParams - The video params to be filled in with default settings
        /// NOTE: The width and height members of the vidParams MUST be set by the caller
        /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
        static TTVSDK_API TTV_ErrorCode TTV_GetDefaultParams(TTV_VideoParams* vidParams);
 
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_PollTasks - Polls all currently executing async tasks and if they've finished calls their callbacks
        static TTVSDK_API TTV_ErrorCode TTV_PollTasks();
 
 
    

        //////////////////////////////////////////////////////////
        /// \brief TTV_RequestAuthToken - Request an authentication key based on the provided username and password.
        /// \param[in] authParams - Authentication parameters
        /// \param[in] callback - The callback function to be called when the request is completed
        /// \param[in] userData - Optional pointer to be passed through to the callback function
        /// \param[in/out] authToken - The authentication token to be written to
        static TTVSDK_API TTV_ErrorCode TTV_RequestAuthToken(const TTV_AuthParams* authParams,
                                                      void* userData,
                                                      TTV_AuthToken* authToken);
 
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_GetIngestServers - Get the list of available ingest servers
        /// \param[in] authToken - The authentication token previously obtained
        /// \param[in] userData - Optional pointer to be passed through to the callback function

        static TTVSDK_API TTV_ErrorCode TTV_GetIngestServers(const TTV_AuthToken* authToken,
                                                              void* userData);


        //////////////////////////////////////////////////////////
        /// \brief TTV_Login - Use the given auth token to log into the associated channel. This MUST be called
        /// \brief before starting to stream and returns information about the channel. If the callback is called
        /// \brief with a failure error code, the most likely cause is an invalid auth token and you need
        /// \param[in] authToken - The authentication token previously obtained
        /// \param[in] userData - Optional pointer to be passed through to the callback function
        static TTVSDK_API TTV_ErrorCode TTV_Login(const TTV_AuthToken* authToken,
                                                    void* userData);
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_Start - Starts streaming
        /// \param[in] videoParams - Output stream video paramaters
        /// \param[in] audioParams - Output stream audio parameters
        static TTVSDK_API TTV_ErrorCode TTV_Start(const TTV_VideoParams* videoParams,
                                           const TTV_AudioParams* audioParams);
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_SubmitVideoFrame - Submit a video frame to be added to the video stream
        /// \param[in] frameBuffer - Pointer to the frame buffer. This buffer will be considered "locked" until the callback is called.
        /// \param[in] callback - The callback function to be called when buffer is no longer needed
        /// \param[in] userData - Optional pointer to be passed through to the callback function
        /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
        static TTVSDK_API TTV_ErrorCode TTV_SubmitVideoFrame(const uint8_t* frameBuffer,
                                                      void* userData);
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_SendMetaData - Send some metadata to Twitch
        /// \param[in] metaData - The metadata to send. This must be in valid JSON format
        static TTVSDK_API TTV_ErrorCode TTV_SendMetaData(const char* metaData);
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_GetVolume - Get the volume level for the given device
        /// \param[in] device - The device to get the volume for
        /// \param[out] volume - The volume level (0.0 to 1.0)
        static TTVSDK_API TTV_ErrorCode TTV_GetVolume(TTV_AudioDeviceType device, float* volume);
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_SetVolume - Set the volume level for the given device
        /// \param[in] device - The device to set the volume for
        /// \param[in] volume - The volume level (0.0 to 1.0)
        static TTVSDK_API TTV_ErrorCode TTV_SetVolume(TTV_AudioDeviceType device, float volume);
 
     
        //////////////////////////////////////////////////////////
        /// \brief TTV_Stop - Stops the stream
        /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
        static TTVSDK_API TTV_ErrorCode TTV_Stop();


        //////////////////////////////////////////////////////////
        /// \brief TTV_Shutdown - Shut down the Twitch Broadcasting SDK
        /// \return - TTV_EC_SUCCESS if function succeeds; error code otherwise
        static TTVSDK_API TTV_ErrorCode TTV_Shutdown();

        //////////////////////////////////////////////////////////
        /// \brief TTV_FreeIngestList - Must be called after getting the ingest servers to free the allocated list of server info
        /// \param[in] ingestList - Pointer to the TTV_IngestList struct to be freed
        static TTVSDK_API TTV_ErrorCode TTV_FreeIngestList(TTV_IngestList* ingestList);

        //////////////////////////////////////////////////////////
        /// \brief TTV_GetStreamInfo - Returns stream-related information from Twitch
        /// \param[in] authToken - The authentication token previously obtained
        /// \param[in] callback - The callback function to be called when user info is retrieved
        /// \param[in] userData - Optional pointer to be passed through to the callback function
        /// \param[out] streamInfo - The stream-related information.

        static TTVSDK_API TTV_ErrorCode TTV_GetStreamInfo(const TTV_AuthToken* authToken,
                                                            void* userData);


        //////////////////////////////////////////////////////////
        /// \brief TTV_GetArchivingState - Returns whether video recording is enabled for the channel and
        /// \brief if not, a cure URL that lets the user to enable recording for the channel
        /// \param[in] authToken - The authentication token previously obtained
        /// \param[in] callback - The callback function to be called when user info is retrieved
        /// \param[in] userData - Optional pointer to be passed through to the callback function
        /// \param[out] archivingState - The channel archiving state

        static TTVSDK_API TTV_ErrorCode TTV_GetArchivingState(const TTV_AuthToken* authToken,
                                                                void* userData);
                                                   
        //////////////////////////////////////////////////////////
        /// \brief TTV_Pause - Pause the video. The SDK will generate frames (e.g. a pause animation)
        /// \brief util TTV_Unpause is called. Audio will continue to be captured/streamed; you can
        /// \brief mute audio by setting the volume to <= 0. Submitting a video frame will unpause the stream.
        static TTVSDK_API TTV_ErrorCode TTV_PauseVideo();

        //////////////////////////////////////////////////////////
        /// \brief TTV_IsScreenSizeEqual -see if our broadcasting dimensions have changed
        static bool TTV_IsScreenSizeEqual(uint32_t width, uint32_t height);

		static void TTV_SetRendererType(int render_type);
		static bool TTV_IsOriginEncoder();
    private:

        //////////////////////////////////////////////////////////
        /// \brief Internal state switch API calls
        static void ResetTwitch();
        static void SetBroadcastRunning(bool state);
        static void updateGlobalBroadcastStats();
        
        //////////////////////////////////////////////////////////
        /// \brief Set native win32 volume
        /// \param[in] device - The device to set the volume for
        /// \param[in] volume - The volume level (0.0 to 1.0)
        static void SetWin32Volume(TTV_AudioDeviceType device, float volume);

        //////////////////////////////////////////////////////////
        /// \brief Check for audio device presense
        /// \param[in] device - The device that we want to enumerate
        /// \return - true - device is present
        static bool IsAudioDevicePresent(TTV_AudioDeviceType device);


        //////////////////////////////////////////////////////////
        /// \brief Callback function to set internal twitch states
 
		static void bufferUnlockCallback(const uint8_t* buffer, void* userData);   
        static void requestAuthTokenCallback(TTV_ErrorCode result, void* userData);
        static void loginCallback(TTV_ErrorCode result, void* userData);
        static void loginCallbackChannelOnly(TTV_ErrorCode result, void* userData);
        static void getIngestServersCallback(TTV_ErrorCode result, void* userData);
        static void getStreamInfoCallback(TTV_ErrorCode result, void* userData);
        static void getArchivingStateCallback(TTV_ErrorCode result, void* userData);
        static void setStreamInfoCallback(TTV_ErrorCode result, void* userData);

        static void twitchAPIStartStatusCallback(TTV_ErrorCode result, void* userData);
        static void twitchAPIStopStatusCallback(TTV_ErrorCode result, void* userData);

        // requests default to TTV_WRN_NOMOREDATA
        static TTV_ErrorCode mRequestAuthTokenStatus;
        static TTV_ErrorCode mBufferUnlockStatus;
        static TTV_ErrorCode mLoginStatus;
        static TTV_ErrorCode mGetChannelStatus;
        static TTV_ErrorCode mIngestServersStatus;
        static TTV_ErrorCode mUserInfoStatus;
        static TTV_ErrorCode mGetStreamInfoStatus;
        static TTV_ErrorCode mArchivingStateStatus;
        static TTV_ErrorCode mSetStreamInfoStatus;
        static TTV_ErrorCode mTwitchAPIStartStatus;
        static TTV_ErrorCode mTwitchAPIStopStatus;

        static TTV_IngestList mIngestList;
        static TTV_IngestServer mServer;
        static TTV_StreamInfo mStreamInfo;
        static TTV_ChannelInfo mChannelInfo;
        static TTV_ArchivingState mArchivingState;

        static TTV_VideoParams mVideoParams;
        static TTV_AudioParams mAudioParams;
        static uint32_t mInputWidth;
        static uint32_t mInputHeight;


        static bool isBroadCastingInitiated;
        static bool isStreamReady;
        static bool isBroadCastingRunning;
        static bool mInitialized;
        static bool mSuppressFollowupErrors;

        static HMODULE getTwitchDll();
        static HMODULE mTwitchDll;
        static wchar_t mTwitchDllFolder[_MAX_PATH];
        static TTV_AuthToken mAuthToken;
        static int mStreamWidth;
        static int mStreamHeight;
        
        static float mMicVolume;
        static float mGameVolume;
        static bool mMicMuted;
        static bool mGameMuted;
        static bool mCaptureMicEnabled;
		static bool mUseOptEncoder;

        static volatile bool mIsShuttingDown;
        static tPixelBufferList mPixelBufferList;
        static EA::Thread::Futex mPixelBufferListMutex;
        static int mUsedPixelBuffers;
        
        static int mLastErrorCode;
        static ERROR_CATEGORY mLastErrorCategory;

		static uint32_t mFrameCount;	// for encoder
		static bool mUseOriginEncoder;
		static int mRendererType;

        static EA::Thread::Futex mTwitchAPILock;                     
 
        class BroadcastThread : public EA::Thread::IRunnable
        {
        public:
            BroadcastThread();
            virtual ~BroadcastThread();

            virtual bool Init(); 
            virtual bool Shutdown();
            void ResetTimer();
        protected:
            EA::Thread::Thread          mThread;                     
            volatile bool               mbThreadShouldQuit;         // Used as a message to our polling thread.
            int                         mPollThreadTimeCheckSeconds;// Time between polls by the thread. See class constructor for the default value.
            EA::Thread::ThreadTime      mTimeNow; // milliseconds
            EA::Thread::ThreadTime      mTimeToPoll;
            EA::Thread::Futex           mBroadcastThreadLock;                     
            virtual intptr_t Run(void* pContext);   // EAThread run function.
        };

        static BroadcastThread mBroadcastStallCheck;
#endif
    };
}
#endif //__TWITCHMANAGER_H__