/*-----------------------------------------------------------------------------
 * Copyright (c) 2010 Electronic Arts Inc. All rights reserved.
 *---------------------------------------------------------------------------*/
#include "stdafx.h"
#include "OriginSDK/OriginTypes.h"
#include <string>
#include <assert.h>
#include <iterator>
#include <chrono>

#include "OriginSDK/OriginSDK.h"
#include "OriginSDKimpl.h"
#include "OriginErrorFunctions.h"
#include "OriginDebugFunctions.h"

#include <ReaderCommon.h>
#include <WriterCommon.h>
#include <XmlDocument.h>
#include <lsx.h>

#include "OriginLSXRequest.h"
#include "OriginLSXEvent.h"
#include "OriginLSXEnumeration.h"
#include "OriginEventClasses.h"
#include "encryption/md5.h"
#include "encryption/SimpleEncryption.h"
#include "OriginSDKTrials.h"

#ifdef ORIGIN_PC
#include "Windows/OriginSDKwindows.h"
#elif defined ORIGIN_MAC
#include "Mac/OriginSDKmac.h"
#else
#error Platform not supported yet.
#endif

#define ORIGINSDK_START_ORIGIN
//#define ORIGINSDK_START_IGO

#define MAX_WAIT_TIME_FOR_PROCESS_START 30 //seconds
#define ENABLE_TRANSACTION_ID			1

namespace Origin
{

    OriginSDK * OriginSDK::s_pSDK = 0;
    OriginSDK * OriginSDK::s_pSDKInstances[ORIGIN_MAX_INSTANCES] = {0};
    int OriginSDK::s_SDKInstanceCount = 0;

    void *OriginSDK::operator new(size_t size)
    {
        return Origin::AllocFunc(size);
    }

    void OriginSDK::operator delete(void *p)
    {
        Origin::FreeFunc(p);
    }

    OriginSDK::OriginSDK() :
		m_refCount(0),
		m_messageThreadStop(false),
		m_Flags(0),
		m_lsxPort(ORIGIN_LSX_PORT)
	{
        memset(m_eventHandlers, 0, sizeof(m_eventHandlers));

		SimpleEncryption random;

		// Shuffle the random number generator.
		random.srand(std::chrono::high_resolution_clock::now().time_since_epoch().count() & 0x7FFF);

		// Initialize JKISS random number generator.
		m_TransactionX = random.rand();
		while ((m_TransactionY = random.rand()) == 0);
		m_TransactionZ = random.rand();
		m_TransactionC = random.rand() % 698769068 + 1;

        Origin::FreezeMemoryAllocators();
    }

    OriginSDK::~OriginSDK()
    {
        StopMessageThread();
		m_Trials.Stop();

        s_pSDK = NULL;

        Origin::ThawMemoryAllocators();
    }

    void OriginSDK::ConfigureSDK(lsx::GetConfigResponseT &config)
    {
        m_Services.resize(lsx::FACILITY_ITEM_COUNT);

        for(unsigned int i = 0; i < config.Services.size(); i++)
        {
            lsx::ServiceT &service = config.Services[i];

            if(service.Facility >= 0 && service.Facility < lsx::FACILITY_ITEM_COUNT && !service.Name.empty() && service.Name[0] != 0)
            {
                m_Services[service.Facility] = service.Name;
            }
        }
    }


    const char *OriginSDK::GetService(lsx::FacilityT facility)
    {
        if(facility >= 0 && facility < lsx::FACILITY_ITEM_COUNT)
        {
            return m_Services[facility].c_str();
        }
        return NULL;
    }

    OriginErrorT OriginSDK::Create(int32_t flags, uint16_t lsxPort, OriginStartupInputT& input, OriginStartupOutputT *pOutput)
    {
        if(s_SDKInstanceCount == ORIGIN_MAX_INSTANCES)
        {
            return ORIGIN_ERROR_TOO_MANY_INSTANCES;
        }

        s_pSDK = new OriginSDK();
        s_pSDK->AddRef();
        return s_pSDK->Initialize(flags, lsxPort, input, pOutput);
    }

    OriginErrorT OriginSDK::SetActiveInstance(OriginInstanceT instance)
    {
        if(instance != ((OriginInstanceT)NULL))
        {
            OriginSDK * pThis = reinterpret_cast<OriginSDK *>(instance);

            for(int i = 0; i < s_SDKInstanceCount; i++)
            {
                if(pThis == s_pSDKInstances[i])
                {
                    s_pSDK = pThis;
                    return REPORTERROR(ORIGIN_SUCCESS);
                }
            }
        }
        return ORIGIN_ERROR_INVALID_ARGUMENT;
    }

    OriginSDK * OriginSDK::Get()
    {
        assert(s_pSDK != NULL);
        return s_pSDK;
    }

    bool OriginSDK::Exists()
    {
        return s_pSDK != 0;
    }

    OriginErrorT OriginSDK::AddRef()
    {
        uint32_t cnt = ++m_refCount;

        if(cnt == 1)
        {
            AddInstanceToInstanceList();
        }

        return cnt == 1 ? REPORTERROR(ORIGIN_SUCCESS) : REPORTERROR(ORIGIN_WARNING_SDK_ALREADY_INITIALIZED);
    }

    OriginErrorT OriginSDK::Release()
    {
        uint32_t cnt = --m_refCount;
        if(cnt == 0)
        {
			// Make sure the instance cannot be reactivated
			RemoveInstanceFromInstanceList();

			// Make sure we are no longer serving requests from other threads.
			if (this == s_pSDK)
				s_pSDK = NULL;

            OriginErrorT err = Shutdown();

            delete this;
            return REPORTERROR(err);
        }
        else
            return REPORTERROR(ORIGIN_WARNING_SDK_STILL_RUNNING);
    }


    uint32_t TranslateVersion(const char *pVersion)
    {
        if(pVersion == NULL)
            return 0;

        int a, b, c, d;

        sscanf(pVersion, "%d,%d,%d,%d", &a, &b, &c, &d);

        return ORIGIN_VERSION(a, b, c, d);
    }

    bool OriginSDK::Authenticate(const char *auth, OriginStartupInputT& input)
    {
        // Generate a key to send to client
        md5_state_t md5state;
        md5_init( &md5state );
#ifdef ORIGIN_PC
        DWORD currentMillis = GetTickCount();
#elif defined ORIGIN_MAC
        struct timeval tv;
        gettimeofday(&tv, NULL);
        long currentMillis = tv.tv_usec;
#endif
        md5_append( &md5state, ( const md5_byte_t* )&currentMillis, ( int ) sizeof(currentMillis) );

        unsigned char digest[16];
        md5_finish( &md5state, ( md5_byte_t* ) digest );

        // Key to authenticate against when client responds with this key encrypted.
        AllocString key;
        for ( int i = 0; i < 16; i++ )
        {
            char buf[3];
            sprintf(buf, "%02x", digest[i] );
            key.append( buf );
        }

        IObjectPtr< LSXRequest<lsx::ChallengeResponseT, lsx::ChallengeAcceptedT> > req(
            new LSXRequest<lsx::ChallengeResponseT, lsx::ChallengeAcceptedT>
                ("EALS", this));

        SimpleEncryption se;

        lsx::ChallengeResponseT &r = req->GetRequest();

        r.response		= se.encrypt(auth);
        r.ContentId		= input.ContentId;
        r.Title			= input.Title;
        r.MultiplayerId	= input.MultiplayerId;
        r.Language		= input.Language;
        r.Version		= input.SdkVersionOverride == NULL ? ORIGIN_SDK_VERSION_STR : input.SdkVersionOverride;
        r.key			= key;

        uint32_t challengeTimeout = input.OriginLaunchChallengeTimeoutSecs ? input.OriginLaunchChallengeTimeoutSecs * 1000 : ORIGIN_LSX_TIMEOUT;
        if (req->Execute(challengeTimeout))
        {
            // Here decrypt key returned by client and test it matches original key sent.
            lsx::ChallengeAcceptedT &challengeResponse = req->GetResponse();
            SimpleEncryption encryption;
            AllocString decryptedKey = encryption.decrypt( challengeResponse.response );

            m_connection.SetUseEncrypted(true);
#if(ORIGIN_SDK_VERSION == 3)
            m_connection.SetSeed(((int)(challengeResponse.response[0]) << 8) + challengeResponse.response[1]);
#else
            m_connection.SetSeed(0);
#endif

            return (decryptedKey == key);
        }
        return false;
    }

    OriginErrorT OriginSDK::Initialize(int32_t flags, uint16_t lsxPort, OriginStartupInputT& input, OriginStartupOutputT *pOutput)
    {
        m_Flags = flags;
        m_ContentId = input.ContentId;
        m_lsxPort = lsxPort == 0 ? ORIGIN_LSX_PORT : lsxPort;

        if(!IsOriginInstalled())
            return REPORTERROR(ORIGIN_ERROR_CORE_NOT_INSTALLED);

#if defined(ORIGIN_DEVELOPMENT)
        bool isRunning = IsOriginProcessRunning();
        if ((input.OriginLaunchSetting == OriginStartupInputT::OriginLaunchSetting_Always) || (!isRunning))
#else
        if (!IsOriginProcessRunning())
#endif	
        {
#ifdef ORIGINSDK_START_ORIGIN
            // If the game insists on not starting return that Origin is not running, and terminate.
#if defined(ORIGIN_DEVELOPMENT)
            if (m_Flags & ORIGIN_FLAG_DO_NOT_START_ORIGIN || input.OriginLaunchSetting == OriginStartupInputT::OriginLaunchSetting_Never)
#else
            if (m_Flags & ORIGIN_FLAG_DO_NOT_START_ORIGIN)
#endif	
                return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);

#if defined(ORIGIN_DEVELOPMENT)
            if ((input.OriginLaunchSetting == OriginStartupInputT::OriginLaunchSetting_Always) && (!isRunning))
            {
                input.OriginLaunchSetting = OriginStartupInputT::OriginLaunchSetting_Detect;
            }
#endif			

            OriginErrorT error;
            error = StartOrigin(input);
            if(error != ORIGIN_SUCCESS)
                return error;
#else
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
#endif
        }

        // The event we want to listen to.
        IObjectPtr< LSXEvent<lsx::ChallengeT> > Challenge(
            new LSXEvent<lsx::ChallengeT>
                ("EALS", this));

        // Start listening to this event.
        OriginSDK::HandlerKey key = Challenge->Register(ORIGIN_LSX_TIMEOUT);

        StartMessageThread();

        OriginErrorT err = ORIGIN_ERROR_CORE_NOTLOADED;

        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        
        // Try to connect to core.
        int retryCount = input.OriginLaunchRetryCount ? input.OriginLaunchRetryCount : MAX_WAIT_TIME_FOR_PROCESS_START;
        uint32_t challengeTimeout = input.OriginLaunchChallengeTimeoutSecs ? input.OriginLaunchChallengeTimeoutSecs * 1000 : ORIGIN_LSX_TIMEOUT;
        uint32_t sleepTime = input.OriginLaunchRetrySleepTime ? input.OriginLaunchRetrySleepTime : 1000;

        for(int i = 0; i < retryCount; i++)
        {
            // Check if we can connect with core.
            if(m_connection.Connect(m_lsxPort))
            {
                // Wait for the event to occur, or timeout.
                if (Challenge->Wait(challengeTimeout))
                {
                    m_OriginVersion = Challenge->GetEvent().version;
                    uint32_t version = TranslateVersion(m_OriginVersion.c_str());

                    // Store the version in the output record, so even when we fail we know what version we were talking to.
                    if(pOutput != NULL)
                    {
                        pOutput->Version = m_OriginVersion.c_str();
                        pOutput->Instance = reinterpret_cast<OriginInstanceT>(this);
                    }

                    // Check whether the Origin version is matching the version we expect.
                    if(version < ORIGIN_MIN_ORIGIN_VERSION)
                    {
                        err = ORIGIN_ERROR_CORE_INCOMPATIBLE_VERSION;
                        break;
                    }

                    // Get the challenge key, and let the connection authenticate itself.
                    if (Authenticate(Challenge->GetEvent().key.c_str(), input))
                    {
                        // Do not change EbisuSDK to OriginSDK, as that will prevent older SDK version from talking to Origin. [Unless both EbisuSDK, and OriginSDK service areas exist.]
                        IObjectPtr< LSXRequest<lsx::GetConfigT, lsx::GetConfigResponseT> > req(
                            new LSXRequest<lsx::GetConfigT, lsx::GetConfigResponseT>
                                ("EbisuSDK", this));

                        if(req->Execute(ORIGIN_LSX_TIMEOUT))
                        {
                            if(req->GetErrorCode() == ORIGIN_SUCCESS)
                            {
                                ConfigureSDK(req->GetResponse());
                            }
                            else
                            {
                                err = req->GetErrorCode();
                                break;
                            }
                        }
                        else
                        {
                            err = ORIGIN_ERROR_LSX_NO_RESPONSE;
                            break;
                        }

                        InitializeEvents();

                        OriginProfileT profile;
                        OriginHandleT handle;
                        if(GetProfileSync(0, ORIGIN_LSX_TIMEOUT, &profile, &handle) == ORIGIN_SUCCESS)
                        {
                            SetDefaultUser(profile.UserId);
                            SetDefaultPersona(profile.PersonaId);
                            DestroyHandle(handle);
                        }

                        char buffer[32];
                        size_t len = 32;

                        if(GetSettingSync(ORIGIN_SETTINGS_IS_IGO_ENABLED, buffer, &len) == ORIGIN_SUCCESS)
                        {
#ifdef ORIGIN_PC
                            if(_stricmp(buffer, "true") == 0)
#elif defined ORIGIN_MAC
                            if(strcasecmp(buffer, "true") == 0)
#endif
                            {
#ifdef ORIGINSDK_START_IGO
                                // This will load the IGO dll into the process.
                                err = StartIGO(Challenge.GetEvent().build == "debug");
                                break;
#else
                                // Try to load the IGO, but don't care if we fail.
                                StartIGO(Challenge->GetEvent().build == "debug");
#endif
                            }
                        }

                        if((flags & ORIGIN_FLAG_ENABLE_TRIAL_THREAD) == ORIGIN_FLAG_ENABLE_TRIAL_THREAD)
                        {
                            len = 32;
                            if(GetGameInfoSync(ORIGIN_GAME_INFO_FREE_TRIAL, buffer, &len) == ORIGIN_SUCCESS)
                            {
#ifdef ORIGIN_PC
                                if(_stricmp(buffer, "true") == 0)
#elif defined ORIGIN_MAC
                                if(strcasecmp(buffer, "true") == 0)
#endif
                                {
									m_Trials.Start();
                                }
                            }
                        }

                        err = ORIGIN_SUCCESS;
                        break;
                    }
                    err = ORIGIN_ERROR_CORE_AUTHENTICATION_FAILED;
                    break;
                }
                else
                {
                    err = Challenge->GetErrorCode();
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
        
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        double elapsedSeconds = std::chrono::duration<double> (end - start).count();

        std::string debugMsg("Elapsed time trying to connect to Origin: ");     
        debugMsg.append(std::to_string(elapsedSeconds));
        REPORTDEBUG(ORIGIN_LEVEL_3, debugMsg.c_str());

        DeregisterMessageHandler(key);

        return REPORTERROR(err);
    }

    OriginErrorT OriginSDK::OriginLogout(int userIndex, OriginErrorSuccessCallback callback, void * pContext)
    {
        if (callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::LogoutT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::LogoutT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_SDK), this, &OriginSDK::OriginLogoutConvertData, callback, pContext));

        req->GetRequest().UserIndex = userIndex;

        if (req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            SetPersonaId(userIndex, 0);
            SetUserId(userIndex, 0);
     
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::OriginLogoutConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }

    OriginErrorT OriginSDK::OriginLogoutSync(int userIndex)
    {
        IObjectPtr< LSXRequest<lsx::LogoutT, lsx::ErrorSuccessT, const char *, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::LogoutT, lsx::ErrorSuccessT, const char *, OriginErrorSuccessCallback>
            (GetService(lsx::FACILITY_SDK), this));

        req->GetRequest().UserIndex = userIndex;

        if (req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            SetPersonaId(userIndex, 0);
            SetUserId(userIndex, 0);

            return REPORTERROR(req->GetResponse().Code);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::RequestTicket(OriginUserT user, OriginTicketT *ticket, size_t *size)
    {
        if ((user == 0) || (user != GetDefaultUser()))
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
        if((ticket == NULL) || (size == NULL))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetAuthTokenT, lsx::AuthTokenT> > req(
            new LSXRequest<lsx::GetAuthTokenT, lsx::AuthTokenT>
                (GetService(lsx::FACILITY_UTILITY), this));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            size_t len = req->GetResponse().value.size();
            char *buffer = TYPE_NEW(char, len + 1);
            strncpy(buffer, req->GetResponse().value.c_str(), len + 1);

            // Store the ticket as a resource.
            OriginHandleT handle = RegisterResource(OriginResourceContainerT::String, buffer);

            // Return the ticket information to the game.
            *ticket = handle;
            *size = len;

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            return REPORTERROR(req->GetErrorCode());
        }
    }

    OriginErrorT OriginSDK::RequestTicket(OriginUserT user, OriginTicketCallback callback, void * pcontext)
    {
        if ((user == 0) || (user != GetDefaultUser()))
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);
        if(callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetAuthTokenT, lsx::AuthTokenT, const char *, OriginTicketCallback> > req(
            new LSXRequest<lsx::GetAuthTokenT, lsx::AuthTokenT, const char *, OriginTicketCallback>
                (GetService(lsx::FACILITY_UTILITY), this, &OriginSDK::RequestTicketConvertData, callback, pcontext));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::RequestTicketConvertData(IXmlMessageHandler * /*pHandler*/, const char *&pTicket, size_t &length, lsx::AuthTokenT &response)
    {
        pTicket = response.value.c_str();
        length = response.value.length();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::RequestAuthCodeSync(OriginUserT user, const OriginCharT * clientId, const OriginCharT * scope, OriginHandleT *authcode, size_t *size, const bool appendAuthSource)
    {
        if((clientId == NULL) || (clientId[0] == 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if((authcode == NULL) || (size == NULL))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetAuthCodeT, lsx::AuthCodeT> > req(
            new LSXRequest<lsx::GetAuthCodeT, lsx::AuthCodeT>
                (GetService(lsx::FACILITY_UTILITY), this));

        req->GetRequest().UserId = user;
        req->GetRequest().ClientId = clientId;
        req->GetRequest().AppendAuthSource = appendAuthSource;
        if(scope)
        {
            req->GetRequest().Scope = scope;
        }

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            size_t len = req->GetResponse().value.size();
            char *buffer = TYPE_NEW(char, len + 1);
            strncpy(buffer, req->GetResponse().value.c_str(), len + 1);

            // Store the ticket as a resource.
            OriginHandleT handle = RegisterResource(OriginResourceContainerT::String, buffer);

            // Return the ticket information to the game.
            *authcode = handle;
            *size = len;

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            return REPORTERROR(req->GetErrorCode());
        }
    }

    OriginErrorT OriginSDK::RequestAuthCode(OriginUserT user, const OriginCharT * clientId, const OriginCharT * scope, OriginAuthCodeCallback callback, void * pcontext, uint32_t timeout, const bool appendAuthSource)
    {
        if((clientId == NULL) || (clientId[0] == 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetAuthCodeT, lsx::AuthCodeT, const char *, OriginAuthCodeCallback> > req(
            new LSXRequest<lsx::GetAuthCodeT, lsx::AuthCodeT, const char *, OriginAuthCodeCallback>
                (GetService(lsx::FACILITY_UTILITY), this, &OriginSDK::RequestAuthCodeConvertData, callback, pcontext));

        req->GetRequest().UserId = user;
        req->GetRequest().ClientId = clientId;
        req->GetRequest().AppendAuthSource = appendAuthSource;

        if(scope)
        {
            req->GetRequest().Scope = scope;
        }

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::RequestAuthCodeConvertData(IXmlMessageHandler * /*handler*/, const char *&authcode, size_t &size, lsx::AuthCodeT &response)
    {
        authcode = response.value.c_str();
        size = response.value.length();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetSettingBuildRequest(lsx::GetSettingT& data, enumSettings settingId)
    {
        if(((unsigned int)settingId) >= lsx::SETTINGTYPE_ITEM_COUNT)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        data.SettingId = (lsx::SettingTypeT) settingId;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetSetting(enumSettings settingId, OriginSettingCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::GetSettingT, lsx::GetSettingResponseT, const char*, OriginSettingCallback> > req(
            new LSXRequest<lsx::GetSettingT, lsx::GetSettingResponseT, const char*, OriginSettingCallback>
                (GetService(lsx::FACILITY_SDK), this, &OriginSDK::GetSettingConvertData, callback, pContext));

        FILL_REQUEST_CHECK(GetSettingBuildRequest(req->GetRequest(), settingId));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::GetSettingSync(enumSettings settingId, OriginCharT * pSetting, size_t *length)
    {
        if((pSetting == NULL) || (*length == 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetSettingT, lsx::GetSettingResponseT> > req(
            new LSXRequest<lsx::GetSettingT, lsx::GetSettingResponseT>
                (GetService(lsx::FACILITY_SDK), this));

        FILL_REQUEST_CHECK(GetSettingBuildRequest(req->GetRequest(), settingId));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            size_t len = req->GetResponse().Setting.size();
            // make sure we have enough space in the user allocated string
            if(len >= *length)
            {
                // Return the space needed when there isn't enough space in the buffer.
                *length = len + 1;
                return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);
            }   

            strncpy(pSetting, req->GetResponse().Setting.c_str(), *length);
            *length = len;

            return REPORTERROR(ORIGIN_SUCCESS);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::GetSettingConvertData(IXmlMessageHandler * /*handler*/, const OriginCharT *& setting, size_t &size, lsx::GetSettingResponseT &response)
    {
        setting = response.Setting.c_str();
        size = response.Setting.length();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetGameInfoBuildRequest(lsx::GetGameInfoT& data, enumGameInfo gameInfoId)
    {
        if(((unsigned int)gameInfoId) >= lsx::GAMEINFOTYPE_ITEM_COUNT)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        data.GameInfoId = (lsx::GameInfoTypeT) gameInfoId;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetGameInfo(enumGameInfo gameInfoId, OriginSettingCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::GetGameInfoT, lsx::GetGameInfoResponseT, const char*, OriginSettingCallback> > req(
            new LSXRequest<lsx::GetGameInfoT, lsx::GetGameInfoResponseT, const char*, OriginSettingCallback>
                (GetService(lsx::FACILITY_SDK), this, &OriginSDK::GetGameInfoConvertData, callback, pContext));

        FILL_REQUEST_CHECK(GetGameInfoBuildRequest(req->GetRequest(), gameInfoId));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::GetGameInfoSync(enumGameInfo gameInfoId, OriginCharT * pInfo, size_t *length)
    {
        if((pInfo == NULL) || (*length == 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetGameInfoT, lsx::GetGameInfoResponseT> > req(
            new LSXRequest<lsx::GetGameInfoT, lsx::GetGameInfoResponseT>
                (GetService(lsx::FACILITY_SDK), this));

        FILL_REQUEST_CHECK(GetGameInfoBuildRequest(req->GetRequest(), gameInfoId));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            size_t len = req->GetResponse().GameInfo.size();
            // make sure we have enough space in the user allocated string
            if(len >= *length)
            {
                *length = len + 1;
                return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);
            }

            strncpy(pInfo, req->GetResponse().GameInfo.c_str(), *length);
            *length = len;

            return REPORTERROR(ORIGIN_SUCCESS);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::GetGameInfoConvertData(IXmlMessageHandler * /*handler*/, const OriginCharT *& setting, size_t &size, lsx::GetGameInfoResponseT &response)
    {
        setting = response.GameInfo.c_str();
        size = response.GameInfo.length();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetAllGameInfo(OriginGameInfoCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::GetAllGameInfoT, lsx::GetAllGameInfoResponseT, OriginGameInfoT, OriginGameInfoCallback> > req(
            new LSXRequest<lsx::GetAllGameInfoT, lsx::GetAllGameInfoResponseT, OriginGameInfoT, OriginGameInfoCallback>
                (GetService(lsx::FACILITY_SDK), this, &OriginSDK::GetGameInfoConvertData, callback, pContext));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::GetAllGameInfoSync(OriginGameInfoT* info, OriginHandleT* handle)
    {
        IObjectPtr< LSXRequest<lsx::GetAllGameInfoT, lsx::GetAllGameInfoResponseT> > req(
            new LSXRequest<lsx::GetAllGameInfoT, lsx::GetAllGameInfoResponseT>
                (GetService(lsx::FACILITY_SDK), this));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            StringContainer * strings = new StringContainer();

            const lsx::GetAllGameInfoResponseT &response = req->GetResponse();

            *handle = RegisterResource(OriginResourceContainerT::StringContainer, strings);

            strings->push_back(response.Languages);
            strings->push_back(response.InstalledVersion);
            strings->push_back(response.InstalledLanguage);
            strings->push_back(response.AvailableVersion);
            strings->push_back(response.DisplayName);
            strings->push_back(response.EntitlementSource);

            info->Expiration = response.Expiration;
            info->FreeTrial = response.FreeTrial;
            info->Languages = (*strings)[0].c_str();
            info->FullGamePurchased = response.FullGamePurchased;
			info->FullGameReleased = response.FullGameReleased;
			info->FullGameReleaseDate = response.FullGameReleaseDate;
            info->HasExpiration = response.HasExpiration;
            info->SystemTime = response.SystemTime;
            info->UpToDate = response.UpToDate;
            info->InstalledVersion = (*strings)[1].c_str();
            info->InstalledLanguage = (*strings)[2].c_str();
            info->AvailableVersion = (*strings)[3].c_str();
            info->DisplayName = (*strings)[4].c_str();
            info->MaxGroupSize = response.MaxGroupSize;
            info->EntitlementSource = (*strings)[5].c_str();

            return REPORTERROR(ORIGIN_SUCCESS);
        }

        return REPORTERROR(req->GetErrorCode());

    }

    OriginErrorT OriginSDK::GetGameInfoConvertData(IXmlMessageHandler * /*handler*/, OriginGameInfoT& info, size_t &size, lsx::GetAllGameInfoResponseT &response)
    {
        size = sizeof(OriginGameInfoT);

        info.Expiration = response.Expiration;
        info.FreeTrial = response.FreeTrial;
        info.FullGamePurchased = response.FullGamePurchased;
		info.FullGameReleased = response.FullGameReleased;
		info.FullGameReleaseDate = response.FullGameReleaseDate;
        info.Languages = response.Languages.c_str();
		info.HasExpiration = response.HasExpiration;
        info.SystemTime = response.SystemTime;
        info.UpToDate = response.UpToDate;
        info.InstalledVersion = response.InstalledVersion.c_str();
        info.InstalledLanguage = response.InstalledLanguage.c_str();
        info.AvailableVersion = response.AvailableVersion.c_str();
        info.DisplayName = response.DisplayName.c_str();
        info.MaxGroupSize = response.MaxGroupSize;
        info.EntitlementSource = response.EntitlementSource.c_str();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetSettings(OriginSettingsCallback callback, void* pContext)
    {
        IObjectPtr< LSXRequest<lsx::GetSettingsT, lsx::GetSettingsResponseT, OriginSettingsT, OriginSettingsCallback> > req(
            new LSXRequest<lsx::GetSettingsT, lsx::GetSettingsResponseT, OriginSettingsT, OriginSettingsCallback>
                (GetService(lsx::FACILITY_SDK), this, &OriginSDK::GetSettingsConvertData, callback, pContext));

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::GetSettingsSync(OriginSettingsT* info, OriginHandleT* handle)
    {
        IObjectPtr< LSXRequest<lsx::GetSettingsT, lsx::GetSettingsResponseT> > req(
            new LSXRequest<lsx::GetSettingsT, lsx::GetSettingsResponseT>
                (GetService(lsx::FACILITY_SDK), this));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            StringContainer * strings = new StringContainer();

            const lsx::GetSettingsResponseT &response = req->GetResponse();

            *handle = RegisterResource(OriginResourceContainerT::StringContainer, strings);

            strings->push_back(response.Environment);
            strings->push_back(response.Language);

            info->Environment = (*strings)[0].c_str();
            info->IsIGOAvailable = response.IsIGOAvailable;
            info->IsIGOEnabled = response.IsIGOEnabled;
            info->Language = (*strings)[1].c_str();
            info->IsTelemetryEnabled = response.IsTelemetryEnabled;
            info->IsManualOffline = response.IsManualOffline;

            return REPORTERROR(ORIGIN_SUCCESS);
        }

        return REPORTERROR(req->GetErrorCode());

    }
    OriginErrorT OriginSDK::GetSettingsConvertData(IXmlMessageHandler * /*handler*/, OriginSettingsT& info, size_t &size, lsx::GetSettingsResponseT &response)
    {
        size = sizeof(OriginSettingsT);

        info.Environment = response.Environment.c_str();
        info.IsIGOAvailable = response.IsIGOAvailable;
        info.IsIGOEnabled = response.IsIGOEnabled;
        info.Language = response.Language.c_str();
        info.IsTelemetryEnabled = response.IsTelemetryEnabled;
        info.IsManualOffline = response.IsManualOffline;

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::CheckOnline(int8_t *pbOnline)
    {
        if(pbOnline == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(m_connection.IsConnected())
        {
            OriginErrorT err = ORIGIN_SUCCESS;

            IObjectPtr< LSXRequest<lsx::GetInternetConnectedStateT, lsx::InternetConnectedStateT> > req(
                new LSXRequest<lsx::GetInternetConnectedStateT, lsx::InternetConnectedStateT>
                    (GetService(lsx::FACILITY_UTILITY), this));

            if(req->Execute(ORIGIN_LSX_TIMEOUT))
            {
                *pbOnline = (int8_t)req->GetResponse().connected;
            }
            else
            {
                err = req->GetErrorCode();
            }

            return REPORTERROR(err);
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
        }
    }


    OriginErrorT OriginSDK::GoOnline(uint32_t timeout, OriginErrorSuccessCallback callback, void * context)
    {
        if(m_connection.IsConnected())
        {
            IObjectPtr< LSXRequest<lsx::GoOnlineT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback> > req(
                new LSXRequest<lsx::GoOnlineT, lsx::ErrorSuccessT, OriginErrorT, OriginErrorSuccessCallback>
                    (GetService(lsx::FACILITY_SDK), this, &OriginSDK::GoOnlineConvertData, callback, context));

            if(req->ExecuteAsync(timeout))
            {
                return REPORTERROR(ORIGIN_SUCCESS);
            }
            else
            {
                OriginErrorT ret = req->GetErrorCode();
                return REPORTERROR(ret);
            }
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
        }
    }

    OriginErrorT OriginSDK::GoOnlineConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t &size, lsx::ErrorSuccessT & response)
    {
        size = 0;

        return response.Code;
    }


    OriginErrorT OriginSDK::CheckPermission(OriginUserT user, enumPermission permission)
    {
        if (user == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_USER);

        if(((unsigned int)permission) >= lsx::PERMISSIONTYPE_ITEM_COUNT)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CheckPermissionT, lsx::CheckPermissionResponseT> > req(
            new LSXRequest<lsx::CheckPermissionT, lsx::CheckPermissionResponseT>
                (GetService(lsx::FACILITY_PERMISSION), this));

        lsx::CheckPermissionT &r = req->GetRequest();

        r.UserId = user;
        r.PermissionId = (lsx::PermissionTypeT)permission;

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            return req->GetResponse().Access;
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::Update()
    {
        //if we are dispatching an incoming message, do not try to handle expired handlers
        {
			std::lock_guard<std::mutex> lock(m_dispatchIncomingMsgLock);
            // should we bind this function to a particular thread and require being called from there?
            ExpireTimedoutHandlers();
        }

        // all periodic processing should be called from here...
        DispatchClientCallbacks();

        // If OriginSDK no longer has a connection to Origin return an error message. This message is for simplifying error handling in the game.
        if(!IsConnected())
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);

        // should we check how frequently we've been called and complain if it isn't often enough?
        return ORIGIN_SUCCESS;
    }

    bool OriginSDK::IsHandleValid(OriginHandleT hHandle)
    {
        return m_resourceMap.find(hHandle) != m_resourceMap.end();
    }


    OriginErrorT OriginSDK::DestroyHandle(OriginHandleT handle)
    {
        OriginResourceContainerT resourceContainer;
        resourceContainer.type = OriginResourceContainerT::String;
        resourceContainer.handle = NULL;

        ResourceMap::iterator i;
        bool found = false;
        {
            std::lock_guard<std::recursive_mutex> msgLock(m_mapLock);
            i = m_resourceMap.find(handle);
            if(i != m_resourceMap.end())
            {
                found = true;
                resourceContainer = i->second;
                m_resourceMap.erase(i);
            }
        }

        if(found)
        {
            OriginErrorT err = ORIGIN_SUCCESS;

            switch(resourceContainer.type)
            {
                case OriginResourceContainerT::String:
                    {
                        TYPE_DELETE((char *)(resourceContainer.handle));
                        break;
                    }
                case OriginResourceContainerT::Enumerator:
                    {

                        IEnumerator *pIEnumerator = (Origin::IEnumerator *)(resourceContainer.handle);

                        if(!pIEnumerator->IsReady())
                            err = ORIGIN_WARNING_SDK_ENUMERATOR_TERMINATED;

                        // Remove the enumerator from the handler map.
                        {
                            std::lock_guard<std::recursive_mutex> mapLock(m_mapLock);

                            for (MessageHandlerMap::iterator j = m_handlerMap.begin(); j != m_handlerMap.end(); j++)
                            {
                                if((IXmlMessageHandler *)(j->second) == pIEnumerator)
                                {
                                    err = ORIGIN_WARNING_SDK_ENUMERATOR_IN_USE;
                                    m_handlerMap.erase(j);
                                    break;
                                }
                            }
                        }

                        // Remove the handle from the callback map.
                        {
                            std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());

                            for(CallbackQueue::iterator j = m_callbackQueue.begin(); j != m_callbackQueue.end(); j++)
                            {
                                if((IXmlMessageHandler *)(*j) == pIEnumerator)
                                {
                                    m_callbackQueue.erase(j);
                                    break;
                                }
                            }
                        }

                        pIEnumerator->Release();

                        break;
                    }
                case OriginResourceContainerT::StringContainer:
                    {
                        delete (StringContainer *)(resourceContainer.handle);
                        break;
                    }
                case OriginResourceContainerT::HandleContainer:
                    {
                        HandleContainer * Handles = (HandleContainer *)(resourceContainer.handle);

                        for(HandleContainer::iterator j = Handles->begin(); j != Handles->end(); j++)
                        {
                            DestroyHandle(*j);
                        }

                        delete Handles;
                        break;
                    }
                case OriginResourceContainerT::CustomDestructor:
                {
                    resourceContainer.destructor(resourceContainer.handle);
                    break;
                }
                default:
                    err = ORIGIN_ERROR_SDK_INVALID_RESOURCE;
                    break;
            }

            return REPORTERROR(err);
        }
        return REPORTERROR(ORIGIN_ERROR_INVALID_HANDLE);
    }


    OriginHandleT OriginSDK::RegisterResource (OriginResourceContainerT::ResourceTypeT resourcetype, void * pResource)
    {
        OriginResourceContainerT resource;
        resource.type = resourcetype;
        resource.handle = (OriginResourceT)pResource;

        std::lock_guard<std::recursive_mutex> msgLock(m_mapLock);
        m_resourceMap[resource.handle] = resource;

        if(resourcetype == OriginResourceContainerT::Enumerator)
        {
            ((IEnumerator *)pResource)->AddRef();
        }

        return resource.handle;
    }

    OriginHandleT OriginSDK::RegisterCustomResource(void * pResource, CustomDestructorFunc customDestructor)
    {
        OriginResourceContainerT resource;
        resource.type = OriginResourceContainerT::CustomDestructor;
        resource.destructor = customDestructor;
        resource.handle = (OriginResourceT)pResource;

        std::lock_guard<std::recursive_mutex> msgLock(m_mapLock);
        m_resourceMap[resource.handle] = resource;

        return resource.handle;
    }



    OriginErrorT OriginSDK::Shutdown()
    {
        // Close down the worker thread.

        if(m_connection.Disconnect())
        {
            bool warnOnNotAllReleased = false;

			StopMessageThread();
			m_Trials.Stop();

			{   // Empty the pending callback list.
				std::lock_guard<std::mutex> messageLock(m_connection.theMessageLock());

                m_callbackQueue.clear();
			}

            ShutdownEvents();

            {   // Delete resources.
                std::lock_guard<std::recursive_mutex> mapLock(m_mapLock);

                while(m_resourceMap.size() > 0)
                {
                    DestroyHandle(m_resourceMap.begin()->first);
                    warnOnNotAllReleased = true;
                }
            }

            if (warnOnNotAllReleased)
                return REPORTERROR(ORIGIN_ERROR_SDK_NOT_ALL_RESOURCES_RELEASED);
            else
                return REPORTERROR(ORIGIN_SUCCESS);
        }

        return REPORTERROR(ORIGIN_ERROR_SDK_NOT_INITIALIZED);
    }

    bool OriginSDK::IsOriginProcessRunning()
    {
        return FindProcessOrigin() != 0;
    }

	uint64_t OriginSDK::GetNewTransactionId()
    {
#if ENABLE_TRANSACTION_ID
		// Formula taken from: http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf
		unsigned long long t;
		m_TransactionX = 314527869 * m_TransactionX + 1234567;
		m_TransactionY ^= m_TransactionY << 5; m_TransactionY ^= m_TransactionY >> 7; m_TransactionY ^= m_TransactionY << 22;
		t = 4294584393ULL * m_TransactionZ + m_TransactionC; m_TransactionC = static_cast<unsigned int>(t >> 32); m_TransactionZ = static_cast<unsigned int>(t);
		uint32_t tt = static_cast<uint32_t>(m_TransactionX + m_TransactionY + m_TransactionZ);

		// There is a modification to the output random number to guarantee non-repeating for at least 256 draws.
		return (((++m_TransactionCount ^ 0x85D) << 12) + (tt & 0xFFF) + ((tt & 0xFFFFF000) << 8)) & 0x7FFFFFFF;
#else
        return 0;
#endif
    }

    OriginSDK::HandlerKey OriginSDK::GetKeyFromTypeAndTransaction (uint32_t typeNameHash, uint64_t transactionId)
    {
        HandlerKey result;
        if (transactionId == 0)	// if not for a specific transaction then use typeName hash in high 32 bits as the key
            result = (uint64_t)typeNameHash << 32;
        else
            result = transactionId;
        return result;
    }

    OriginSDK::HandlerKey OriginSDK::RegisterMessageHandler(IXmlMessageHandler * handler, uint64_t transactionId)
    {
        std::lock_guard<std::recursive_mutex> mapLock(m_mapLock);

        HandlerKey key = this->GetKeyFromTypeAndTransaction(handler->GetPacketIdHash(), transactionId);
        m_handlerMap[key] = handler;
        return key;
    }

    IObjectPtr< IXmlMessageHandler> OriginSDK::DeregisterMessageHandler(HandlerKey key)
    {
        std::lock_guard<std::recursive_mutex> mapLock(m_mapLock);

        MessageHandlerMap::iterator iter = m_handlerMap.find(key);
        if(iter != m_handlerMap.end())
        {
            IObjectPtr< IXmlMessageHandler> handler = (*iter).second;
            m_handlerMap.erase(iter);
            return handler;
        }
        else
        {
            return NULL;
        }
    }

    IObjectPtr< IXmlMessageHandler> OriginSDK::LookupMessageHandler(HandlerKey key)
    {
        std::lock_guard<std::recursive_mutex> mapLock(m_mapLock);

        // attempt retrieval for this transaction
        MessageHandlerMap::iterator iter = m_handlerMap.find(key);
        if (iter != m_handlerMap.end())
        {
            return (*iter).second;
        }

        // attempt retrieval for the key as a typename
        key &= 0xFFFFFFFF00000000ULL;
        iter = m_handlerMap.find(key);
        if (iter != m_handlerMap.end())
        {
            return (*iter).second;
        }
        return NULL;
    }

    void OriginSDK::ExpireTimedoutHandlers()
    {
        std::lock_guard<std::recursive_mutex> mapLock(m_mapLock);

        // Check if any message has times out
restart:
        for(MessageHandlerMap::iterator i = m_handlerMap.begin(); i != m_handlerMap.end(); i++)
        {
            IObjectPtr< IXmlMessageHandler> handler = i->second;

            if(handler->IsTimedOut() || handler->GetErrorCode() < ORIGIN_SUCCESS)
            {
                // Notify the callback we have a problem.
                if(handler->HasCallback())
                {
                    std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());
                    m_callbackQueue.push_back(handler);
                }

                // Remove the handler from the map as it is in an error state.
                m_handlerMap.erase(i);

                // As the map has changed geometry we have to restart the iteration.
                goto restart;
            }
        }
    }

    void OriginSDK::DispatchClientCallbacks()
    {

        while(m_callbackQueue.size()>0)
        {
            IObjectPtr< IXmlMessageHandler> handler;

            {
                std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());

                if(m_callbackQueue.size() > 0)
                {
                    handler = m_callbackQueue.front();
                    m_callbackQueue.pop_front();
                }
            }

            if(handler != NULL)
			    handler->DoCallback();
		}
    }

    void OriginSDK::DispatchIncomingMessages()
    {
        AllocString msg;
        while (this->RetrieveLsxMessage(msg))
        {
            RapidXmlParser xmlDoc;
            xmlDoc.Parse(msg.c_str());
            xmlDoc.Root();

            AllocString rootName(xmlDoc.GetName());

            if (rootName == "LSX")
            {
                uint64_t transactionId = 0ULL;
                uint32_t messageTypeHash = 0;

                if(xmlDoc.FirstChild())	// wrapper node
                {
                    const char *messageTransactionId = xmlDoc.GetAttribute("id") ? xmlDoc.GetAttributeValue() : "0";
 
                    Read(messageTransactionId, transactionId);

                    if(transactionId == 0ULL)
                    {
                        if(xmlDoc.FirstChild())	// payload node
                        {
                            const char* payload_typename = xmlDoc.GetName();
                            //OutputDebugStringA("Dispatching payload=");
                            //OutputDebugStringA(payload_typename);
                            //OutputDebugStringA("\n");
                            messageTypeHash = GetHash(payload_typename);		// payload typename
                        }
                    }
                }

                HandlerKey key = this->GetKeyFromTypeAndTransaction(messageTypeHash, transactionId);
                {
                    std::lock_guard<std::mutex> dispatchLock(m_dispatchIncomingMsgLock);
                    IObjectPtr< IXmlMessageHandler> handler = this->LookupMessageHandler(key);

                    if (handler != NULL)
                    {
                        OriginErrorT err = handler->HandleMessage(&xmlDoc);

                        if(handler->HasCallback())
                        {
                            std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());
                            m_callbackQueue.push_back(handler);
                        }

                        if (err != ORIGIN_SUCCESS)
                            REPORTERROR(err);
                    }
                    else
                    {
                        // ignore messages with no handler;  do we want to report them somehow?
                    }
                }
            }
            else
            {
                // ignore non-LSX messages; do we want to report them somehow?
            }
        }
    }

    OriginErrorT OriginSDK::SendXmlDocument (INodeDocument * doc)
    {
        doc->Root();	// send whole document, not just current node!
        this->SendLsxMessage(doc->GetXml().c_str());
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::SendXml(const AllocString & doc)
    {
        this->SendLsxMessage(doc.c_str());
        return REPORTERROR(ORIGIN_SUCCESS);
    }

    void OriginSDK::StartMessageThread()
    {
        if(!m_messageThread.joinable())
        {
			m_messageThreadStop = false;
			m_messageThread = std::thread([&] { Run(); });
        }
    }

    void OriginSDK::StopMessageThread()
    {
        if(m_messageThread.joinable())
        {
			{
				std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());
				m_messageThreadStop = true;
			}

            m_connection.theCondition().notify_one();
			m_messageThread.join();
        }
    }

    void OriginSDK::SendLsxMessage(const char* msg)
    {
		{
			std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());

			REPORTDEBUG(ORIGIN_LEVEL_2, msg);

			m_Outgoing.push_back(AllocString(msg));
		}
		m_connection.theCondition().notify_one();
    }

    bool OriginSDK::RetrieveLsxMessage(AllocString & msg)
    {
        std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());
        if (m_Incoming.empty())
        {
            msg.clear();
            return false;
        }
        else
        {
            msg = m_Incoming.front();
            m_Incoming.pop_front();

            // Test just in case we read a null
            if (!msg.length())
            {
                msg.clear();
                return false;
            }

            REPORTDEBUG(ORIGIN_LEVEL_2, msg.c_str());
            return true;
        }
    }

    bool OriginSDK::IsThreadRunning()
    {
        return m_messageThread.joinable();
    }


    void OriginSDK::Run()
    {

        while (!m_messageThreadStop)
        {
            //OutputDebugStringA("S");
            // check for data to read, data to write or wait for a signal

			{
				std::unique_lock<std::mutex> lock(m_connection.theMessageLock());
				m_connection.theCondition().wait(lock, [&]() -> bool {return IsReady(); });
			}

            AllocString msg;
            bool newMessagesArrived = false;

            // read & enqueue any incoming messages
            while (m_connection.Read(msg))
            {
                std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());
                m_Incoming.push_back(msg);

                newMessagesArrived = true;
            }

            // attempt to write all queued messages
            bool doneSending = false;
            while (!doneSending)
            {
                {   // retrieve next message, if there is one
                    std::lock_guard<std::mutex> msgLock(m_connection.theMessageLock());
                    if (m_Outgoing.empty())
                    {
                        msg.clear();
                        doneSending = true;
                    }
                    else
                    {
                        msg = m_Outgoing.front();
                        m_Outgoing.pop_front();
                        doneSending = m_Outgoing.empty();
                    }
                }

                if (!msg.empty())
                    doneSending = !m_connection.Write(msg);
            }

            if(newMessagesArrived)
            {
                // asynchronous processing of incoming messages could be handled here instead of in Update:
                this->DispatchIncomingMessages();
            }
        }
    }

    OriginErrorT OriginSDK::QueryImage(const OriginCharT * pImageId, uint32_t Width, uint32_t Height, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT * pHandle)
    {
        if(pImageId == NULL || pImageId[0] == 0 || Width == 0 || Height == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(pHandle == NULL && callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);


        LSXEnumeration<lsx::QueryImageT, lsx::QueryImageResponseT, OriginImageT> *req =
            new LSXEnumeration<lsx::QueryImageT, lsx::QueryImageResponseT, OriginImageT>
                (GetService(lsx::FACILITY_RESOURCES), this, &OriginSDK::QueryImageConvertData, callback, pContext);

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        lsx::QueryImageT &r = req->GetRequest();

        r.ImageId = pImageId;
        r.Width = Width;
        r.Height = Height;

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            if (pHandle != NULL) *pHandle = NULL;
            DestroyHandle(h);
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryImageSync(const OriginCharT * pImageId, uint32_t Width, uint32_t Height, size_t * pTotal, uint32_t timeout, OriginHandleT * pHandle)
    {
        if(pHandle == NULL || pImageId == NULL || pImageId[0] == 0 || pTotal == NULL || Width == 0 || Height == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryImageT, lsx::QueryImageResponseT, OriginImageT> *req =
            new LSXEnumeration<lsx::QueryImageT, lsx::QueryImageResponseT, OriginImageT>
                (GetService(lsx::FACILITY_RESOURCES), this, &OriginSDK::QueryImageConvertData);

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        lsx::QueryImageT &r = req->GetRequest();

        r.ImageId = pImageId;
        r.Width = Width;
        r.Height = Height;

        if(req->Execute(timeout, pTotal))
        {
            return REPORTERROR(req->GetResponse().Result);
        }

        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryImageConvertData(IEnumerator * /*pEnumerator*/, OriginImageT * pData, size_t index, size_t count, lsx::QueryImageResponseT & response)
    {
        if((pData == NULL) && (count > 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        for(uint32_t i = 0; i < count; i++)
        {
            OriginImageT &dest = pData[i];
            const lsx::ImageT &src = response.Images[i + index];

            dest.ImageId = src.ImageId.c_str();
            dest.Width = src.Width;
            dest.Height = src.Height;
            dest.ResourcePath = src.ResourcePath.c_str();
        }

        return REPORTERROR(response.Result);
    }

    OriginErrorT OriginSDK::QueryUserIdSync(const OriginCharT * pIdentifier, size_t *pTotalCount, uint32_t timeout, OriginHandleT * pHandle)
    {
        if(pIdentifier == NULL || pIdentifier[0] == 0 || pTotalCount == NULL || pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::GetUserProfileByEmailorEAIDT, lsx::GetUserProfileByEmailorEAIDResponseT, OriginFriendT> *req =
            new LSXEnumeration<lsx::GetUserProfileByEmailorEAIDT, lsx::GetUserProfileByEmailorEAIDResponseT, OriginFriendT>
        (GetService(lsx::FACILITY_GET_USERID), this, &OriginSDK::QueryUserIdConvertData);

        lsx::GetUserProfileByEmailorEAIDT &r = req->GetRequest();

        r.KeyWord = pIdentifier;

        *pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

        // Dispatch the command, and wait for an reply.
        if(req->Execute(timeout, pTotalCount))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryUserId(const OriginCharT * pIdentifier, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT * pHandle)
    {
        if(pIdentifier == NULL || pIdentifier[0] == 0)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);
        if(callback == NULL && pHandle == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);


        LSXEnumeration<lsx::GetUserProfileByEmailorEAIDT, lsx::GetUserProfileByEmailorEAIDResponseT, OriginFriendT> *req =
            new LSXEnumeration<lsx::GetUserProfileByEmailorEAIDT, lsx::GetUserProfileByEmailorEAIDResponseT, OriginFriendT>
        (GetService(lsx::FACILITY_GET_USERID), this, &OriginSDK::QueryUserIdConvertData, callback, pContext);

        lsx::GetUserProfileByEmailorEAIDT &r = req->GetRequest();

        r.KeyWord = pIdentifier;

        OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL)
        {
            *pHandle = h;
        }

        // Dispatch the command.
        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::QueryUserIdConvertData(IEnumerator * /*pEnumerator*/, OriginFriendT * pData, size_t index, size_t count, lsx::GetUserProfileByEmailorEAIDResponseT & response)
    {
        if((pData == NULL) && (count > 0))
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        for(uint32_t i = 0; i < count; i++)
        {
            OriginFriendT &destFriend = pData[i];
            lsx::UserT &srcUser = response.User[i + index];

            ConvertData(destFriend, srcUser);
        }

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::GetUTCTime( OriginTimeT * time )
    {
        if(time == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetUTCTimeT, lsx::GetUTCTimeResponseT> > req(
            new LSXRequest<lsx::GetUTCTimeT, lsx::GetUTCTimeResponseT>
                (GetService(lsx::FACILITY_SDK), this));

        if(req->Execute(ORIGIN_LSX_TIMEOUT))
        {
            *time = req->GetResponse().Time;

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::BroadcastCheckStatus(int8_t *pbStatus)
    {
        if(pbStatus == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(m_connection.IsConnected())
        {
            OriginErrorT err = ORIGIN_SUCCESS;

            IObjectPtr< LSXRequest<lsx::GetBroadcastStatusT, lsx::BroadcastStatusT> > req(
                new LSXRequest<lsx::GetBroadcastStatusT, lsx::BroadcastStatusT>
                    (GetService(lsx::FACILITY_SDK), this));

            if(req->Execute(ORIGIN_LSX_TIMEOUT))
            {
                *pbStatus = (int8_t)req->GetResponse().status;
            }
            else
            {
                err = req->GetErrorCode();
            }

            return REPORTERROR(err);
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
        }
    }


    OriginErrorT OriginSDK::BroadcastStart()
    {
        if(m_connection.IsConnected())
        {
            OriginErrorT err = ORIGIN_PENDING;

            IObjectPtr< LSXRequest<lsx::BroadcastStartT, lsx::ErrorSuccessT> > req(
                new LSXRequest<lsx::BroadcastStartT, lsx::ErrorSuccessT>
                    (GetService(lsx::FACILITY_SDK), this));

            if(!req->Execute(ORIGIN_LSX_TIMEOUT))
            {
                err = req->GetErrorCode();
            }

            return REPORTERROR(err);
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
        }
    }

    OriginErrorT OriginSDK::BroadcastStop()
    {
        if(m_connection.IsConnected())
        {
            OriginErrorT err = ORIGIN_PENDING;

            IObjectPtr< LSXRequest<lsx::BroadcastStopT, lsx::ErrorSuccessT> > req(
                new LSXRequest<lsx::BroadcastStopT, lsx::ErrorSuccessT>
                    (GetService(lsx::FACILITY_SDK), this));

            if(!req->Execute(ORIGIN_LSX_TIMEOUT))
            {
                err = req->GetErrorCode();
            }

            return REPORTERROR(err);
        }
        else
        {
            return REPORTERROR(ORIGIN_ERROR_CORE_NOTLOADED);
        }
    }

    OriginErrorT OriginSDK::SendGameMessage(const OriginCharT * gameId, const OriginCharT * message, OriginErrorSuccessCallback callback, void * pContext)
    {
        if(gameId == NULL || gameId[0] == 0 || message == NULL || message[0] == 0 || callback == NULL)
            return ORIGIN_ERROR_INVALID_ARGUMENT;

        IObjectPtr< LSXRequest<lsx::SendGameMessageT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
            new LSXRequest<lsx::SendGameMessageT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
                (GetService(lsx::FACILITY_SDK), this, &OriginSDK::SendGameMessageConvertData, callback, pContext));

        lsx::SendGameMessageT &r = req->GetRequest();

        r.GameId = gameId;
        r.Message = message;

        if(req->ExecuteAsync(ORIGIN_LSX_TIMEOUT))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
            OriginErrorT ret = req->GetErrorCode();
            return REPORTERROR(ret);
        }
    }

    OriginErrorT OriginSDK::SendGameMessageConvertData(IXmlMessageHandler * /*pHandler*/, OriginErrorT & /*item*/, size_t & /*size*/, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }


    void OriginSDK::AddInstanceToInstanceList()
    {
        // Add the new instance to the instance list.
        s_pSDKInstances[s_SDKInstanceCount++] = this;
    }

    void OriginSDK::RemoveInstanceFromInstanceList()
    {
        // Remove the instance from the list.
        for(int i = 0; i < s_SDKInstanceCount; i++)
        {
            if(s_pSDKInstances[i] == this)
            {
                s_pSDKInstances[i] = NULL;

                if(s_SDKInstanceCount - i - 1 > 0)
                {
                    memmove(s_pSDKInstances + i, s_pSDKInstances + i + 1, (s_SDKInstanceCount - i - 1) * sizeof(Origin::OriginSDK *));
                }
                s_SDKInstanceCount--;
                break;
            }
        }
    }

    template<> int GetErrorCodeFromResponse(LSXMessage<lsx::ErrorSuccessT> &t)
    { 
        return t->Code; 
    }
}
