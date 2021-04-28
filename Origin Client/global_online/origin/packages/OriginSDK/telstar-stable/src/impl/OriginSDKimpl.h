/*-----------------------------------------------------------------------------
 * Copyright (c) 2010 Electronic Arts Inc. All rights reserved.
 *---------------------------------------------------------------------------*/
#ifndef __ORIGINSDK_IMPL_H__
#define __ORIGINSDK_IMPL_H__

#include "OriginSDK/OriginTypes.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDK/OriginEnums.h"
#include "OriginSDK/OriginEnumeration.h"
#include "OriginSDK/OriginDefines.h"
#include "OriginLSXSupport.h"
#include <string>
#include "OriginMap.h"
#include <deque>
#include <lsx.h>
#include "OriginSDKMemory.h"
#include "OriginSDKTrials.h"

class INodeDocument;

#define ORIGIN_LSX_TIMEOUT 15000
#define ORIGIN_MAX_INSTANCES 16

struct OriginTrialConfig
{
    int ExtendBeforeExpire;     // How much time before the end of a ticket we start requesting a new one in ms.
    int SleepBeforeNukeTime;    // How much time we wait before we nuke the game in ms
    int RetryCount;             // The number of times we try to get a ticket after request failure
    int RetryAfterFail;         // The amount of time we wait before retrying in ms.
};

namespace Origin
{
    class IEventHandler;

/// \ingroup types
/// \brief
///
    typedef intptr_t	OriginResourceT;

    typedef void(*CustomDestructorFunc) (OriginResourceT handle);

    struct OriginResourceContainerT
    {
        enum ResourceTypeT
        {
            Unknown,
            String,
            Enumerator,
            StringContainer,
            HandleContainer,
            CustomDestructor,
        } type;

        OriginResourceT handle;
        CustomDestructorFunc destructor;

        OriginResourceContainerT() : type(Unknown), handle(0), destructor(NULL) {}
    };


    class IObject
    {
    public:
        IObject() : m_ref(0) 
        {
        }
        virtual ~IObject() 
        {
        }

        virtual void AddRef()
        {
            m_ref++;
        }

        virtual void Release()
        {
            if(--m_ref <= 0)
            {
                delete this;
            }
        }

		IObject(const IObject &) = delete;
		IObject(IObject &&) = delete;
		IObject & operator = (const IObject &) = delete;
		IObject & operator = (IObject &&) = delete;

    private:
        std::atomic<uint32_t> m_ref;
    };

    template <typename T> class IObjectPtr
    {
    public:
        IObjectPtr() : m_pT(NULL) {}
        IObjectPtr(T *p) : m_pT(p)
        { 
            if(m_pT) m_pT->AddRef(); 
        }
        IObjectPtr(const IObjectPtr<T> &p) : m_pT(p.m_pT)
        {
            if(m_pT) m_pT->AddRef();
        }
        ~IObjectPtr()
        { 
            if(m_pT) m_pT->Release(); 
        }
        T * operator = (T *p) 
        { 
            if(m_pT) m_pT->Release(); 
            m_pT = p; 
            if(m_pT) m_pT->AddRef(); 
            return m_pT; 
        }
        const IObjectPtr<T> & operator = (const IObjectPtr<T> &p)
        {
            if(m_pT) m_pT->Release();
            m_pT = p.m_pT;
            if(m_pT) m_pT->AddRef();
            return *this;
        }
        T * operator -> () const
        { 
            return m_pT; 
        }

        bool operator == (const T *p) const
        { 
            return m_pT == p; 
        }
        bool operator != (const T *p) const
        { 
            return m_pT != p; 
        }
        operator T * () const
        {
            return m_pT;
        }

    private:
        T * m_pT;
    };

    class IXmlMessageHandler : public IObject      
    {
    public:
		IXmlMessageHandler() {}
        virtual ~IXmlMessageHandler() {}

        virtual uint32_t GetPacketIdHash() = 0;

        /// Called from OriginSDK::DispatchIncomingMessages() to handle each incoming message.  After this call, if deregisterSelf
        /// is set to true, then nothing else will be done with this handler object and it will be removed from the table of
        /// registered handlers.
        /// \param[in] doc The xml document instantiated from the incoming message, current node is the payload
        /// \param[out] deregisterSelf If set to true the dispatch method will remove this handler from the table
        /// \return Return the error code resulting from the handling of this message -- if it is not success it will be propagated to the calling user.
        virtual OriginErrorT HandleMessage(INodeDocument* doc) = 0;

        /// Polling function to check whether the handler received the data.
        virtual bool IsReady() = 0;

        virtual bool IsTimedOut() = 0;

        /// Check whether the MsgHandler has a callback installed.
        virtual bool HasCallback() = 0;

        /// Check the current state of the handle.
        virtual OriginErrorT GetErrorCode() = 0;

        /// When data is available calling this function in OriginUpdate will do a callback to the registered callback function.
        virtual OriginErrorT DoCallback() = 0;

        /// Allocate memory in the handler context
        virtual void StoreMemory(void * pData) = 0;

		IXmlMessageHandler(const IXmlMessageHandler &) = delete;
		IXmlMessageHandler(IXmlMessageHandler &&) = delete;
		IXmlMessageHandler & operator = (const IXmlMessageHandler &) = delete;
		IXmlMessageHandler & operator = (IXmlMessageHandler &&) = delete;
    };


    class IEnumerator : public IXmlMessageHandler
    {
    public:
		IEnumerator() {}
        virtual ~IEnumerator() {}

        virtual bool IsReady() = 0;
        virtual bool IsTimedOut() = 0;

        virtual size_t GetTotalCount() = 0;
        virtual size_t GetAvailableCount() = 0;
        virtual size_t GetTypeSize() = 0;

        virtual bool Wait() = 0;
        virtual OriginErrorT ConvertData(void *pBufPtr, size_t iBufLen, size_t index, size_t count, size_t *pItemCnt) = 0;

		IEnumerator(const IEnumerator &) = delete;
		IEnumerator(IEnumerator &&) = delete;
		IEnumerator & operator = (const IEnumerator &) = delete;
		IEnumerator & operator = (IEnumerator &&) = delete;
	};

    template<typename T> class Container
    {
    public:
        typedef std::vector<T, Allocator<T> >						ContainerT;
        typedef typename std::vector<T, Allocator<T> >::iterator	iterator;

        void *operator new(size_t size)
        {
            return AllocFunc(size);
        }
        void operator delete(void * data)
        {
            FreeFunc(data);
        }

        iterator begin() {return mContainer.begin();}
        iterator end() {return mContainer.end();}
        void reserve(size_t size) { mContainer.reserve(size); }
        void resize(size_t size) { mContainer.resize(size); }
        void push_back(const T & data) { mContainer.push_back(data); }
        void clear() { mContainer.clear(); }

        T & operator [] (size_t index) { return mContainer[index]; }

    private:
        ContainerT mContainer;
    };

    typedef Container<AllocString> StringContainer;
    typedef Container<OriginHandleT> HandleContainer;
    typedef Container<void *> MemoryContainer;
    
    constexpr int SDK_USERS_LENGTH = 4;

    class OriginSDK 
    {
        friend class Origin::Trials;
		friend class PlatformGameTokenInterfaceImpl;
		friend class PlatformUserTokenInterfaceImpl;
	public:
        OriginSDK();
        ~OriginSDK();

        static bool Exists();
        static OriginErrorT Create(int32_t flags, uint16_t lsxPort, OriginStartupInputT& input, OriginStartupOutputT *pOutput);
        static OriginSDK *Get();

    public:
        OriginErrorT AddRef();
        OriginErrorT Release();

    public:
        void *operator new(size_t size);
        void operator delete(void*);

        typedef intptr_t	OriginTicketT;

        OriginUserT GetDefaultUser() const
        {
            return GetUserId(0);
        }

        void SetDefaultUser(OriginUserT userId)
        {
            SetUserId(0, userId);
        }

        OriginPersonaT GetDefaultPersona() const
        {
            return GetPersonaId(0);
        }

        void SetDefaultPersona(OriginPersonaT persona)
        {
            SetPersonaId(0, persona);
        }

        OriginUserT GetUserId(int userIndex) const
        {
            if (userIndex >= 0 && userIndex < SDK_USERS_LENGTH)
                return m_userIdArray[userIndex];

            return m_userIdArray[0];
        }

        void SetUserId(int userIndex, OriginUserT userId)
        {
            if (userIndex >= 0 && userIndex < SDK_USERS_LENGTH)
                m_userIdArray[userIndex] = userId;
        }

        OriginUserT GetPersonaId(int userIndex) const
        {
            if (userIndex >= 0 && userIndex < SDK_USERS_LENGTH)
                return m_personaIdArray[userIndex];

            return m_personaIdArray[0];
        }

        void SetPersonaId(int userIndex, OriginPersonaT persona)
        {
            if (userIndex >= 0 && userIndex < SDK_USERS_LENGTH)
                m_personaIdArray[userIndex] = persona;
        }

        const OriginCharT * GetContentId() { return m_ContentId.c_str(); }

        OriginErrorT OriginLogout(int userIndex, OriginErrorSuccessCallback callback, void * pcontext);
        OriginErrorT OriginLogoutSync(int userIndex);
        OriginErrorT OriginLogoutConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);
         
        OriginErrorT RequestTicket(OriginUserT user, OriginTicketT *ticket, size_t *size);
        OriginErrorT RequestTicket(OriginUserT user, OriginTicketCallback callback, void * pcontext);
        OriginErrorT RequestTicketConvertData(IXmlMessageHandler * handler, const char *&pTicket, size_t &size, lsx::AuthTokenT &response);
        OriginErrorT RequestAuthCodeSync(OriginUserT user, const OriginCharT * clientId, const OriginCharT * scope, OriginHandleT *authcode, size_t *size, const bool appendAuthSource = false);
        OriginErrorT RequestAuthCode(OriginUserT user, const OriginCharT * clientId, const OriginCharT * scope, OriginAuthCodeCallback callback, void * pcontext, uint32_t timeout, const bool appendAuthSource = false);
        OriginErrorT RequestAuthCodeConvertData(IXmlMessageHandler * handler, const char *&authcode, size_t &size, lsx::AuthCodeT &response);

        //Settings and game info
        //Individual setting
        OriginErrorT GetSetting(enumSettings settingId, OriginSettingCallback callback, void* pContext);
        OriginErrorT GetSettingSync(enumSettings settingId, OriginCharT *pSetting, size_t *length);
        OriginErrorT GetSettingBuildRequest(lsx::GetSettingT& data, enumSettings settingId);
        OriginErrorT GetSettingConvertData(IXmlMessageHandler * handler, const OriginCharT *& setting, size_t &size, lsx::GetSettingResponseT &response);
        //Individual Game info
        OriginErrorT GetGameInfo(enumGameInfo gameInfoId, OriginSettingCallback callback, void* pContext);
        OriginErrorT GetGameInfoSync(enumGameInfo gameInfoId, OriginCharT *pInfo, size_t *length);
        OriginErrorT GetGameInfoBuildRequest(lsx::GetGameInfoT& data, enumGameInfo gameInfoId);
        OriginErrorT GetGameInfoConvertData(IXmlMessageHandler * handler, const OriginCharT *& setting, size_t &size, lsx::GetGameInfoResponseT &response);
        //Game info blob
        OriginErrorT GetAllGameInfo(OriginGameInfoCallback callback, void* pContext);
        OriginErrorT GetAllGameInfoSync(OriginGameInfoT* info, OriginHandleT* handle);
        OriginErrorT GetGameInfoConvertData(IXmlMessageHandler * handler, OriginGameInfoT& info, size_t &size, lsx::GetAllGameInfoResponseT &response);
        //settings blob
        OriginErrorT GetSettings(OriginSettingsCallback callback, void* pContext);
        OriginErrorT GetSettingsSync(OriginSettingsT* info, OriginHandleT* handle);
        OriginErrorT GetSettingsConvertData(IXmlMessageHandler * handler, OriginSettingsT& info, size_t &size, lsx::GetSettingsResponseT &response);

        OriginErrorT CheckOnline(int8_t *pbOnline);
        OriginErrorT GoOnline(uint32_t timeout, OriginErrorSuccessCallback callback, void * context);
        OriginErrorT GoOnlineConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT BroadcastCheckStatus(int8_t *pbOnline);
        OriginErrorT BroadcastStart();
        OriginErrorT BroadcastStop();

        OriginErrorT SendGameMessage(const OriginCharT * gameId, const OriginCharT * message, OriginErrorSuccessCallback callback, void * pContext);
        OriginErrorT SendGameMessageConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT Update();

        // Resource Management Interface
        OriginHandleT RegisterResource (OriginResourceContainerT::ResourceTypeT resourceType, void * resource);
        OriginHandleT RegisterCustomResource(void * resource, CustomDestructorFunc customDestructor);
        OriginErrorT DestroyHandle(OriginHandleT handle);

        // Enumerate Functions.
        OriginErrorT ReadEnumeration(OriginHandleT hHandle, void *pBufPtr, size_t bufSize, size_t startIndex, size_t count, size_t *countRead);
        OriginErrorT ReadEnumerationSync(OriginHandleT hHandle, void *pBufPtr, size_t bufSize, size_t startIndex, size_t count, size_t *countRead);
        OriginErrorT GetEnumerateStatus(OriginHandleT hHandle, size_t *total, size_t *available);

        // Event Management Interface
        OriginErrorT RegisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext);
        OriginErrorT UnregisterEventCallback(int32_t iEvents, OriginEventCallback callback, void* pContext);

        // Permission Functions.
        OriginErrorT CheckPermission(OriginUserT user, enumPermission permission);

        // Profile Functions
        OriginErrorT GetProfileSync(uint32_t userIndex, uint32_t timeout, OriginProfileT *pProfile, OriginHandleT * pHandle);
        OriginErrorT GetProfile(uint32_t userIndex, uint32_t timeout, OriginResourceCallback callback, void * pcontext);
        OriginErrorT GetProfileConvertData(IXmlMessageHandler *pHandler, OriginProfileT &profile, size_t &size, lsx::GetProfileResponseT &response);

        // Presence Functions.
        OriginErrorT SubscribePresence(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT SubscribePresenceSync(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, uint32_t timeout);
        OriginErrorT SubscribePresenceConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT UnsubscribePresence(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT UnsubscribePresenceSync(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, uint32_t timeout);
        OriginErrorT UnsubscribePresenceConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT SetPresence(OriginUserT user, enumPresence presence, const OriginCharT *pPresenceString, const OriginCharT* pGamePresenceString, const OriginCharT *pSession, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT SetPresenceSync(OriginUserT user, enumPresence presence, const OriginCharT *pPresenceString, const OriginCharT* pGamePresenceString, const OriginCharT *pSession, uint32_t timeout);
        OriginErrorT SetPresenceConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT GetPresence(OriginUserT user, OriginGetPresenceCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT GetPresenceSync(OriginUserT user, enumPresence * pPresence, OriginCharT *pPresenceString, size_t presenceStringLen, OriginCharT *pGamePresenceString, size_t gamePresenceStringLen, OriginCharT *pSessionString, size_t sessionStringLen, uint32_t timeout);
        OriginErrorT GetPresenceConvertData(IXmlMessageHandler *pHandler, OriginGetPresenceT &presence, size_t &size, lsx::GetPresenceResponseT &response);

        OriginErrorT SetPresenceVisibility(OriginUserT user, bool visible, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT SetPresenceVisibilitySync(OriginUserT user, bool visible, uint32_t timeout);
        OriginErrorT SetPresenceVisibilityConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT GetPresenceVisibility(OriginUserT user, OriginGetPresenceVisibilityCallback callback, void* pContext, uint32_t timeout);
        OriginErrorT GetPresenceVisibilitySync(OriginUserT user, bool * visible, uint32_t timeout);
        OriginErrorT GetPresenceVisibilityConvertData(IXmlMessageHandler *pHandler, bool &item, size_t &size, lsx::GetPresenceVisibilityResponseT &response);

        OriginErrorT QueryPresence(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryPresenceSync(OriginUserT user, const OriginUserT *otherUsers, size_t userCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryPresenceConvertData(IEnumerator *pEnumerator, OriginFriendT *pData, size_t index, size_t count, lsx::QueryPresenceResponseT &response);

        // Friend Functions
        OriginErrorT QueryFriends(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryFriendsSync(OriginUserT user, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryFriendsConvertData(IEnumerator *pEnumerator, OriginFriendT *pData, size_t index, size_t count, lsx::QueryFriendsResponseT &response);

        OriginErrorT QueryAreFriends(OriginUserT user, const OriginUserT *pUserList, size_t iUserCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryAreFriendsSync(OriginUserT user, const OriginUserT *pUserList, size_t iUserCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryAreFriendsConvertData(IEnumerator *pEnumerator, OriginIsFriendT *pData, size_t index, size_t count, lsx::QueryAreFriendsResponseT &response);

        OriginErrorT RequestFriend(OriginUserT user, OriginUserT userToAdd, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext);
        OriginErrorT RequestFriendSync(OriginUserT user, OriginUserT userToAdd, uint32_t timeout);
        OriginErrorT RequestFriendConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT RemoveFriend(OriginUserT user, OriginUserT userToRemove, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext);
        OriginErrorT RemoveFriendSync(OriginUserT user, OriginUserT userToRemove, uint32_t timeout);
        OriginErrorT RemoveFriendConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT AcceptFriendInvite(OriginUserT user, OriginUserT userToAdd, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext);
        OriginErrorT AcceptFriendInviteSync(OriginUserT user, OriginUserT userToAdd, uint32_t timeout);
        OriginErrorT AcceptFriendInviteConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);


        // IGO Functions
        OriginErrorT ShowIGO(bool bShow, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowIGOSync(bool bShow);
        OriginErrorT ShowIGOBuildRequest(lsx::ShowIGOT& data, bool bShow);
        OriginErrorT ShowIGOConvertData(IXmlMessageHandler *pHandler, int &dummy, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT ShowIGOWindow(enumIGOWindow window, OriginUserT user, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowIGOWindowSync(enumIGOWindow window, OriginUserT user);
        OriginErrorT ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user);

        OriginErrorT ShowIGOWindow(enumIGOWindow window, OriginUserT user, OriginUserT target, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowIGOWindowSync(enumIGOWindow window, OriginUserT user, OriginUserT target);
        OriginErrorT ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user, OriginUserT target);

        OriginErrorT ShowIGOWindow(enumIGOWindow window, int32_t iBrowserFeatureFlags, const OriginCharT * pURL, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowIGOWindowSync(enumIGOWindow window, int32_t iBrowserFeatureFlags, const OriginCharT * pURL);
        OriginErrorT ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, int32_t iBrowserFeatureFlags, const OriginCharT * pURL);

        OriginErrorT ShowIGOWindow(enumIGOWindow window, OriginUserT user, const OriginUserT * users, size_t userCount, const OriginCharT * pMessage, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowIGOWindowSync(enumIGOWindow window, OriginUserT user, const OriginUserT * users, size_t userCount, const OriginCharT * pMessage);
        OriginErrorT ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user, const OriginUserT * users, size_t userCount, const OriginCharT * pMessage);

        OriginErrorT ShowIGOWindow(enumIGOWindow window, OriginUserT user, const OriginCharT ** ppArgs, size_t argCount, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowIGOWindowSync(enumIGOWindow window, OriginUserT user, const OriginCharT ** ppArgs, size_t argCount);
        OriginErrorT ShowIGOWindowBuildRequest(lsx::ShowIGOWindowT& data, enumIGOWindow window, OriginUserT user, const OriginCharT ** ppArgs, size_t argCount);
        
        OriginErrorT ShowStoreUI(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowStoreUISync(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount);
        OriginErrorT ShowStoreUIBuildRequest(lsx::ShowIGOWindowT& data, OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount);

        OriginErrorT ShowCodeRedemptionUI(OriginUserT user, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowCodeRedemptionUISync(OriginUserT user);
        OriginErrorT ShowCodeRedemptionUIBuildRequest(lsx::ShowIGOWindowT& data, OriginUserT user);

        OriginErrorT ShowCheckoutUI(OriginUserT user, const OriginCharT *pOfferList[], size_t offerCount, OriginErrorSuccessCallback callback, void* pContext);
        OriginErrorT ShowCheckoutUISync(OriginUserT user, const OriginCharT *pOfferList[], size_t offerCount);
        OriginErrorT ShowCheckoutUIBuildRequest(lsx::ShowIGOWindowT& data, OriginUserT user, const OriginCharT *pOfferList[], size_t offerCount);

        // Image Functions
        OriginErrorT QueryImage(const OriginCharT *pImageId, uint32_t Width, uint32_t Height, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryImageSync(const OriginCharT *pImageId, uint32_t Width, uint32_t Height, size_t * pTotal, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryImageConvertData(IEnumerator *pEnumerator, OriginImageT *pData, size_t index, size_t count, lsx::QueryImageResponseT &response);

        // Commerce Functions
        OriginErrorT SelectStore(uint64_t storeId, uint64_t catalogId, uint64_t eWalletCategoryId, const OriginCharT *pVirtualCurrency, const OriginCharT *pLockboxUrl, const OriginCharT *pSuccessUrl, const OriginCharT * pFailedUrl);

        OriginErrorT GetStore(OriginUserT user, uint64_t storeId, OriginStoreCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT GetStoreSync(OriginUserT user, uint64_t storeId, OriginStoreT ** ppStore, size_t *pNumberOfStores, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT GetStoreConvertData(IXmlMessageHandler *pHandler, OriginStoreT &pStore, size_t &size, lsx::GetStoreResponseT &reponse);

        OriginErrorT GetCatalog(OriginUserT user, OriginCatalogCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT GetCatalogSync(OriginUserT user, OriginCategoryT **pCategories, size_t *pCategoryCount, uint32_t timeout, OriginHandleT *handle);
        OriginErrorT GetCatalogConvertData(IXmlMessageHandler *handler, OriginCategoryT *&pCategories, size_t &size, lsx::GetCatalogResponseT &response);

        OriginErrorT GetWalletBalance(OriginUserT user, const OriginCharT *currency, OriginWalletCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT GetWalletBalanceSync(OriginUserT user, const OriginCharT *currency, int64_t *pBalance, uint32_t timeout);
        OriginErrorT GetWalletBalanceConvertData(IXmlMessageHandler *pHandler, int64_t &balance, size_t &Size, lsx::GetWalletBalanceResponseT &response);

        OriginErrorT QueryCategories(OriginUserT user, const OriginCharT *pParentCategoryList[], size_t parentCategoryCount, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryCategoriesSync(OriginUserT user, const OriginCharT *pParentCategoryList[], size_t parentCategoryCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryCategoriesConvertData(IEnumerator *pEnumerator, OriginCategoryT *pCategories, size_t index, size_t count, lsx::QueryCategoriesResponseT &response);

        OriginErrorT QueryOffers(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryOffersSync(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pMasterTitleIdList[], OriginSizeT MasterTitleIdCount, const OriginCharT *pOfferList[], size_t offerCount, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryOffersConvertData(IEnumerator *pEnumerator, OriginOfferT *pOffers, size_t index, size_t count, lsx::QueryOffersResponseT &response);

        OriginErrorT QueryContent(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT gameIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginEnumerationCallbackFunc callback, void* context, uint32_t timeout, OriginHandleT *handle);
        OriginErrorT QueryContentSync(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT gameIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginSizeT *total, uint32_t timeout, OriginHandleT *handle);
        OriginErrorT QueryContentConvertData(IEnumerator *pEnumerator, OriginContentT *pContent, size_t index, size_t count, lsx::QueryContentResponseT &response);

        OriginErrorT RestartGame(OriginUserT user, enumRestartGameOptions option, OriginErrorSuccessCallback callback, void *context, uint32_t timeout);
        OriginErrorT RestartGameSync(OriginUserT user, enumRestartGameOptions option, uint32_t timeout);
        OriginErrorT RestartGameConvertData(IXmlMessageHandler *pHandler, int &dummy, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT StartGame(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, OriginErrorSuccessCallback callback, void *context, uint32_t timeout);
        OriginErrorT StartGameSync(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, uint32_t timeout);
        OriginErrorT StartGameConvertData(IXmlMessageHandler *pHandler, int &dummy, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT Checkout(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], size_t offerCount, OriginResourceCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT CheckoutSync(OriginUserT user, const OriginCharT * Currency, const OriginCharT *pOfferList[], size_t offerCount, uint32_t timeout);
        OriginErrorT CheckoutConvertData(IXmlMessageHandler *pHandler, int &dummy, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT QueryManifest(OriginUserT user, const OriginCharT* manifest, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryManifestSync(OriginUserT user, const OriginCharT* manifest, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryManifestConvertData(IEnumerator *pEnumerator, OriginItemT *pItems, size_t index, size_t count, lsx::QueryManifestResponseT &response);

        OriginErrorT QueryEntitlements(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pOfferList[], size_t offerCount, const OriginCharT *pItemList[], size_t itemCount, const OriginCharT * pGroup, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryEntitlementsSync(OriginUserT user, const OriginCharT *pCategoryList[], size_t categoryCount, const OriginCharT *pOfferList[], size_t offerCount, const OriginCharT *pItemList[], size_t itemCount, const OriginCharT * pGroup, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryEntitlements(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pItemList[], OriginSizeT itemCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, bool includeExpiredTrialDLC, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryEntitlementsSync(OriginUserT user, const OriginCharT *pOfferList[], OriginSizeT offerCount, const OriginCharT *pItemList[], OriginSizeT itemCount, const OriginCharT *pGroupList[], OriginSizeT groupCount, bool includeChildGroups, bool includeExpiredTrialDLC, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryEntitlementsConvertData(IEnumerator *pEnumerator, OriginItemT *, size_t index, size_t count, lsx::QueryEntitlementsResponseT &response);

        OriginErrorT ConsumeEntitlement(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, OriginEntitlementCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT ConsumeEntitlementSync(OriginUserT user, uint32_t uses, bool bOveruse, OriginItemT *pItem, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT ConsumeEntitlementConvertData(IXmlMessageHandler *pHandler, OriginItemT &item, size_t &size, lsx::ConsumeEntitlementResponseT &response);

        // Recent Players Functions
        OriginErrorT AddRecentPlayers(OriginUserT user, const OriginUserT *pRecentList, size_t recentCount, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);
        OriginErrorT AddRecentPlayersSync(OriginUserT user, const OriginUserT *pRecentList, size_t recentCount, uint32_t timeout);
        OriginErrorT AddRecentPlayersConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        // Chat message
		OriginErrorT SendChatMessage(OriginUserT fromUser, OriginUserT toUser, const OriginCharT * thread, const OriginCharT* message, const OriginCharT* groupId);

        // Group support
        OriginErrorT QueryGroup(OriginUserT user, const OriginCharT * groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle);
        OriginErrorT QueryGroupSync(OriginUserT user, const OriginCharT * groupId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT * handle);
        OriginErrorT QueryGroupConvertData(IEnumerator *pEnumerator, OriginFriendT *pUser, size_t index, size_t count, lsx::QueryGroupResponseT &response);

        OriginErrorT GetGroupInfo(OriginUserT user, const OriginCharT * groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT GetGroupInfoSync(OriginUserT user, const OriginCharT * groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle);
        OriginErrorT GetGroupInfoConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &groupInfo, size_t &size, lsx::GroupInfoT &response);

        OriginErrorT SendGroupGameInvite(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT SendGroupGameInviteSync(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout);
        OriginErrorT SendGroupGameInviteConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT CreateGroup(OriginUserT user, const OriginCharT *groupName, enumGroupType type, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT CreateGroupSync(OriginUserT user, const OriginCharT *groupName, enumGroupType type, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT * handle);
        OriginErrorT CreateGroupConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &groupInfo, size_t &size, lsx::GroupInfoT &response);

        OriginErrorT EnterGroup(OriginUserT user, const OriginCharT *groupId, OriginGroupInfoCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT EnterGroupSync(OriginUserT user, const OriginCharT *groupId, uint32_t timeout, OriginGroupInfoT * groupInfo, OriginHandleT *handle);
        OriginErrorT EnterGroupConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &groupInfo, size_t &size, lsx::GroupInfoT &response);

        OriginErrorT LeaveGroup(OriginUserT user, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT LeaveGroupSync(OriginUserT user, uint32_t timeout);
        OriginErrorT LeaveGroupConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT InviteUsersToGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT InviteUsersToGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout);
        OriginErrorT InviteUsersToGroupConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT RemoveUsersFromGroup(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT RemoveUsersFromGroupSync(OriginUserT user, const OriginUserT *users, OriginSizeT userCount, uint32_t timeout);
        OriginErrorT RemoveUsersFromGroupConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);
        
        OriginErrorT EnableVoip(bool bEnable, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT EnableVoipSync(bool bEnable, uint32_t timeout);
        OriginErrorT EnableVoipConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT GetVoipStatus(OriginVoipStatusCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT GetVoipStatusSync(OriginVoipStatusT * status, uint32_t timeout);
        OriginErrorT GetVoipStatusConvertData(IXmlMessageHandler *pHandler, OriginVoipStatusT &voipStatus, size_t &size, lsx::GetVoipStatusResponseT &response);

        OriginErrorT MuteUser(bool bMute, OriginUserT userId, const OriginCharT *groupId, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT MuteUserSync(bool bMute, OriginUserT userId, const OriginCharT *groupId, uint32_t timeout);
        OriginErrorT MuteUserConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT QueryMuteState(const OriginCharT *groupId, OriginEnumerationCallbackFunc callback, void * pContext, uint32_t timeout, OriginHandleT * handle);
        OriginErrorT QueryMuteStateSync(const OriginCharT *groupId, OriginSizeT * total, uint32_t timeout, OriginHandleT * handle);
        OriginErrorT QueryMuteStateConvertData(IEnumerator *pEnumerator, OriginMuteStateT *pMuteState, size_t index, size_t count, lsx::QueryMuteStateResponseT &response);

        // Invite message
        OriginErrorT SendInvite(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);
        OriginErrorT SendInviteSync(OriginUserT user, const OriginUserT * pUserList, size_t userCount, const OriginCharT * pMessage, uint32_t timeout);
        OriginErrorT SendInviteConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        // Accept Invite
        OriginErrorT AcceptInvite(OriginUserT user, OriginUserT other, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);
        OriginErrorT AcceptInviteSync(OriginUserT user, OriginUserT other, uint32_t timeout);
        OriginErrorT AcceptInviteConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        // Blocked user list
        OriginErrorT QueryBlockedUsers(OriginUserT user, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryBlockedUsersSync(OriginUserT user, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryBlockedUsersConvertData(IEnumerator *pEnumerator, OriginFriendT *pData, size_t index, size_t count, lsx::GetBlockListResponseT &response);

        OriginErrorT BlockUser(OriginUserT user, OriginUserT userToBlock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext);
        OriginErrorT BlockUserSync(OriginUserT user, OriginUserT userToBlock, uint32_t timeout);
        OriginErrorT BlockUserConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT UnblockUser(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout, OriginErrorSuccessCallback callback, void * pContext);
        OriginErrorT UnblockUserSync(OriginUserT user, OriginUserT userToUnblock, uint32_t timeout);
        OriginErrorT UnblockUserConvertData(IXmlMessageHandler *pHandler, OriginErrorT &item, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT QueryUserId(const OriginCharT *pIdentifier, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryUserIdSync(const OriginCharT *pIdentifier, size_t *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryUserIdConvertData(IEnumerator *pEnumerator, OriginFriendT *pData, size_t index, size_t count, lsx::GetUserProfileByEmailorEAIDResponseT &response);

        OriginErrorT GrantAchievement(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementCallback callback, void *pContext);
        OriginErrorT GrantAchievementSync(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementT *achievement, OriginHandleT *handle);
        OriginErrorT GrantAchievementConvertData(IXmlMessageHandler *pHandler, OriginAchievementT &achievement, size_t &size, lsx::AchievementT &response);

        OriginErrorT CreateEventRecord(OriginHandleT * eventHandle);
        OriginErrorT AddEvent(OriginHandleT eventHandle, const OriginCharT * eventName);
        OriginErrorT AddEventParameter(OriginHandleT eventHandle, const OriginCharT *name, const OriginCharT *value);

        OriginErrorT PostAchievementEvents(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);
        OriginErrorT PostAchievementEventsSync(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout);
        OriginErrorT PostAchievementEventsConvertData(IXmlMessageHandler *pHandler, int &dummy, OriginSizeT &size, lsx::ErrorSuccessT &response);

        OriginErrorT PostWincodes(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);
        OriginErrorT PostWincodesSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout);
        OriginErrorT PostWindcodesConvertData(IXmlMessageHandler *pHandler, int &dummy, size_t &size, lsx::ErrorSuccessT &response);

        OriginErrorT QueryAchievements(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryAchievementsSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryAchievementsConvertData(IEnumerator *pEnumerator, OriginAchievementSetT *, size_t index, size_t count, lsx::AchievementSetsT &response);

        // Progressive installation
        OriginErrorT IsProgressiveInstallationAvailable(const OriginCharT *itemId, OriginIsProgressiveInstallationAvailableCallback callback, void * pContext, uint32_t timeout);
        OriginErrorT IsProgressiveInstallationAvailableSync(const OriginCharT *itemId, bool *bAvailable, uint32_t timeout);
        OriginErrorT IsProgressiveInstallationAvailableConvertData(IXmlMessageHandler *pHandler, OriginIsProgressiveInstallationAvailableT &info, OriginSizeT &size, lsx::IsProgressiveInstallationAvailableResponseT &response);

        OriginErrorT AreChunksInstalled(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, OriginAreChunksInstalledCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT AreChunksInstalledSync(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, bool *installed, uint32_t timeout);
        OriginErrorT AreChunksInstalledConvertData(IXmlMessageHandler *pHandler, OriginAreChunksInstalledInfoT &installInfo, OriginSizeT &size, lsx::AreChunksInstalledResponseT &response);

        OriginErrorT QueryChunkStatus(const OriginCharT *itemId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryChunkStatusSync(const OriginCharT *itemId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryChunkStatusConvertData(IEnumerator *pEnumerator, OriginChunkStatusT *chunkStatus, size_t index, size_t count, lsx::QueryChunkStatusResponseT &response);

        OriginErrorT IsFileDownloaded(const OriginCharT *itemId, const char *filePath, OriginIsFileDownloadedCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT IsFileDownloadedSync(const OriginCharT *itemId, const char *filePath, bool *downloaded, uint32_t timeout);
        OriginErrorT IsFileDownloadedConvertData(IXmlMessageHandler *pHandler, OriginDownloadedInfoT &fileStatus, OriginSizeT &size, lsx::IsFileDownloadedResponseT &response);

        OriginErrorT SetChunkPriority(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT SetChunkPrioritySync(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, uint32_t timeout);
        OriginErrorT SetChunkPriorityConvertData(IXmlMessageHandler *pHandler, int &dummy, OriginSizeT &size, lsx::ErrorSuccessT &response);

        OriginErrorT GetChunkPriority(const OriginCharT *itemId, OriginChunkPriorityCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT GetChunkPrioritySync(const OriginCharT *itemId, OriginCharT * itemIdOut, OriginSizeT * itemIdOutSize, int32_t * pChunckIds, OriginSizeT * chunkCount, uint32_t timeout);
        OriginErrorT GetChunkPriorityConvertData(IXmlMessageHandler *pHandler, OriginChunkPriorityInfoT &chunkInfo, OriginSizeT &size, lsx::GetChunkPriorityResponseT &response);

        OriginErrorT QueryChunkFiles(const OriginCharT *itemId, int32_t chunkId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryChunkFilesSync(const OriginCharT *itemId, int32_t chunkId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);
        OriginErrorT QueryChunkFilesConvertData(IEnumerator *pEnumerator, const char **filenames, size_t index, size_t count, lsx::QueryChunkFilesResponseT &response);

        OriginErrorT CreateChunk(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, OriginCreateChunkCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT CreateChunkSync(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, int32_t *pChunkId, uint32_t timeout);
        OriginErrorT CreateChunkConvertData(IXmlMessageHandler *pHandler, int &chunkId, OriginSizeT &size, lsx::CreateChunkResponseT &response);

        OriginErrorT StartDownload(const OriginCharT *itemId, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT StartDownloadSync(const OriginCharT *itemId, uint32_t timeout);
        OriginErrorT StartDownloadConvertData(IXmlMessageHandler *pHandler, int &dummy, OriginSizeT &size, lsx::ErrorSuccessT &response);

        OriginErrorT SetDownloadUtilization(float utilization, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);
        OriginErrorT SetDownloadUtilizationSync(float utilization, uint32_t timeout);
        OriginErrorT SetDownloadUtilizationConvertData(IXmlMessageHandler *pHandler, int &dummy, OriginSizeT &size, lsx::ErrorSuccessT &response);



		OriginErrorT OverlayStateChanged(bool isUp, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
		OriginErrorT OverlayStateChangedSync(bool isUp, uint32_t timeout);
		OriginErrorT OverlayStateChangedConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

		OriginErrorT CompletePurchaseTransaction(uint32_t appId, uint64_t orderId, bool authorized, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
		OriginErrorT CompletePurchaseTransactionSync(uint32_t appId, uint64_t orderId, bool authorized, uint32_t timeout);
		OriginErrorT CompletePurchaseTransactionConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

        OriginErrorT RefreshEntitlements(OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
        OriginErrorT RefreshEntitlementsSync(uint32_t timeout);
        OriginErrorT RefreshEntitlementsConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

        OriginErrorT SetPlatformDLC(DLC* dlc, uint32_t count, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
        OriginErrorT SetPlatformDLCSync(DLC* dlc, uint32_t count, uint32_t timeout);
        OriginErrorT SetPlatformDLCConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

        OriginErrorT DetermineCommerceCurrency(OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
        OriginErrorT DetermineCommerceCurrencySync(uint32_t timeout);
        OriginErrorT DetermineCommerceCurrencyConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

		OriginErrorT SendSteamAchievementErrorTelemetry(bool validStat, bool getStat, bool setStat, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
        OriginErrorT SendSteamAchievementErrorTelemetrySync(bool validStat, bool getStat, bool setStat, uint32_t timeout);
        OriginErrorT SendSteamAchievementErrorTelemetryConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

        OriginErrorT GetUTCTime( OriginTimeT * time );

        OriginErrorT SetSteamLocale(const OriginCharT *language, OriginErrorSuccessCallback callback, void* pContext, uint32_t timeout);
        OriginErrorT SetSteamLocaleSync(const OriginCharT *language, uint32_t timeout);
        OriginErrorT SetSteamLocaleConvertData(IXmlMessageHandler* pHandler, int& dummy, OriginSizeT& size, lsx::ErrorSuccessT& response);

    public:
        // messaging interface -- public so that handlers can be external to this class
        typedef uint64_t HandlerKey;

        // Message handler management
        uint64_t GetNewTransactionId();	///< Generate a new unique transaction id (unique to this session)
        HandlerKey GetKeyFromTypeAndTransaction (uint32_t typeNameHash, uint64_t transactionId);	///< generate a handler's key in the registration table
        HandlerKey RegisterMessageHandler(IXmlMessageHandler* handler, uint64_t transactionId);		///< register a message handler, optionally for a specific transaction;  returns handler's key
        IObjectPtr<IXmlMessageHandler> DeregisterMessageHandler(HandlerKey key);								///< deregisters a message handler specified by a key
        
        // Message document management
        OriginErrorT SendXmlDocument (INodeDocument* doc);	///< Serializes and transmits the document, then destroys the document object
        OriginErrorT SendXml(const AllocString &doc);			///< Transmits the document, then destroys the document object

        bool IsConnected() const {return m_connection.IsConnected();}

        const OriginCharT * GetOriginVersion() {return m_OriginVersion.c_str();}

        const char *GetService(lsx::FacilityT facility);

        static OriginErrorT SetActiveInstance(OriginInstanceT instance);

    public:
        static const OriginCharT * ConvertData(Origin::IXmlMessageHandler *pHandler, const Origin::AllocString &src, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginItemT &Dest, lsx::EntitlementT &Src, bool bCopyStrings);
        static void ConvertData(OriginItemT &Dest, const lsx::EntitlementT &Src);
        static void ConvertData(lsx::EntitlementT &Src, OriginItemT &Dest);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginItemT *dest, std::vector<lsx::EntitlementT, Origin::Allocator<lsx::EntitlementT> > &src, size_t index, size_t count, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginOfferT *dest, std::vector<lsx::OfferT, Origin::Allocator<lsx::OfferT> > &src, size_t index, size_t count, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginContentT *dest, std::vector<lsx::GameT, Origin::Allocator<lsx::GameT> > &src, size_t index, size_t count, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginStoreT &dest, lsx::StoreT &src, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginCatalogT &dest, lsx::CatalogT &src, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginCatalogT *dest, std::vector<lsx::CatalogT, Origin::Allocator<lsx::CatalogT> > &src, size_t index, size_t count, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginCategoryT &dest, lsx::CategoryT &src, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginCategoryT *dest, std::vector<lsx::CategoryT, Origin::Allocator<lsx::CategoryT> > &src, size_t index, size_t count, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginCategoryT *&categories, size_t &count, lsx::CatalogT &catalog, bool bCopyStrings);
        static void ConvertData(OriginFriendT &dest, const lsx::FriendT &src);
        static void ConvertData(OriginFriendT &dest, const lsx::UserT &src);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginAchievementSetT &dest, const lsx::AchievementSetT &src, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginAchievementT *dest, const std::vector<lsx::AchievementT, Origin::Allocator<lsx::AchievementT> > &src, size_t index, size_t count, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginAchievementT &dest, const lsx::AchievementT &src, bool bCopyStrings);
        static void ConvertData(IXmlMessageHandler *pHandler, OriginGroupInfoT &dest, const lsx::GroupInfoT &src, bool bCopyStrings);

        OriginErrorT Initialize(int32_t flags, uint16_t lsxPort, OriginStartupInputT& input, OriginStartupOutputT *pOutput);
        OriginErrorT Shutdown();

		bool IsReady() const 
		{ 
			return !m_Outgoing.empty() || m_connection.HasReadDataToRead() || m_messageThreadStop; 
		}

		OriginSDK(const OriginSDK &) = delete;
		OriginSDK(OriginSDK &&) = delete;
		OriginSDK & operator = (const OriginSDK &) = delete;
		OriginSDK & operator = (OriginSDK &&) = delete;

	protected:
		// This functions are for internal use only an should only be used by the Origin SDK owners. 
		OriginErrorT ExtendTrial(const OriginCharT *requestTicket, int ticketEngine, int * totalTimeRemaining, int * timeGranted, OriginCharT *timeTicket, OriginSizeT * size, OriginTrialConfig &config, uint32_t timeout);
		OriginErrorT RequestLicense(const OriginCharT *requestTicket, int ticketEngine, OriginCharT *license, OriginSizeT * size, uint32_t timeout);
		OriginErrorT InvalidateLicense(uint32_t timeout);

	private:
        bool Authenticate(const char *auth, OriginStartupInputT& input);
        void ConfigureSDK(lsx::GetConfigResponseT &config);

        bool IsOriginInstalled();
        bool IsOriginProcessRunning();

        void DispatchIncomingMessages();					///< Dispatches queued messages, this will empty the pending message queue.
        void DispatchClientCallbacks();						///< Dispatches Messages in the callback queue.
        void ExpireTimedoutHandlers();						///< Check whether the list of handlers contains handlers that have timed out.
        IObjectPtr<IXmlMessageHandler> LookupMessageHandler(HandlerKey key);	///< retrieves the message handler for the specified key

        void InitializeEvents();
        void ShutdownEvents();

        void Run();
        bool IsThreadRunning();
        void StartMessageThread();
        void StopMessageThread();
        void SendLsxMessage(const char* msg);
        bool RetrieveLsxMessage(AllocString& msg);

        // implemented in OriginSDK__platform__.cpp
        OriginErrorT StartOrigin(const OriginStartupInputT& input);
        OriginErrorT StartIGO(bool loadDebugVersion);

        bool IsHandleValid(OriginHandleT hHandle);

        void AddInstanceToInstanceList();
        void RemoveInstanceFromInstanceList();


    private:
        typedef std::map<intptr_t, OriginResourceContainerT, std::less<intptr_t>, Allocator<std::pair<const intptr_t, OriginResourceContainerT>>>               ResourceMap;
        typedef std::map<uint64_t, IObjectPtr<IXmlMessageHandler>, std::less<uint64_t>, Allocator<std::pair<const uint64_t, IObjectPtr<IXmlMessageHandler>>>>	MessageHandlerMap;
        typedef std::deque<AllocString, Allocator<AllocString> >																								MessageQueue;
        typedef std::deque<IObjectPtr<IXmlMessageHandler>, Allocator<IObjectPtr<IXmlMessageHandler> > >															CallbackQueue;

        std::atomic<int32_t>			m_refCount;
        ResourceMap						m_resourceMap;
        MessageHandlerMap				m_handlerMap;

        std::thread						m_messageThread;	///< Origin SDK's internal worker thread
        std::atomic<bool>				m_messageThreadStop;
        std::recursive_mutex			m_mapLock;			///< Lock for resource/handler maps
		std::mutex						m_dispatchIncomingMsgLock;

        Trials							m_Trials;			///< The Thread Object.
        
        LSX								m_connection;		///< Process connection
        MessageQueue					m_Incoming;			///< Incoming queue of xml messages.
        MessageQueue					m_Outgoing;			///< Outgoing queue of xml messages.
        CallbackQueue					m_callbackQueue;	///< Queue for storing objects that are ready to callback to the client.

		// JKISS algorithm used to calculate transaction id.
		uint32_t m_TransactionX = 0, m_TransactionY = 0, m_TransactionZ = 0, m_TransactionC = 0;
		uint64_t m_TransactionCount = 12485;

        Origin::IEventHandler*			m_eventHandlers[ORIGIN_NUM_EVENTS];

        int								m_Flags;			///< Flags to change operation behavior
        uint16_t						m_lsxPort;			///< The port used to communicate through with Origin client
        AllocString						m_OriginVersion;	///< The version of Origin provided during IObject negotiation.
        AllocString						m_ContentId;		///< Remember the content id.
        
        OriginUserT                     m_userIdArray[SDK_USERS_LENGTH]    = { 0 };    ///< Array of nucleusId of the logged in user(s).
        OriginPersonaT                  m_personaIdArray[SDK_USERS_LENGTH] = { 0 };    ///< Array of personaId of the logged in user(s).

        StringContainer 				m_Services;

    private:
		static OriginSDK *				s_pSDK;
		static OriginSDK *				s_pSDKInstances[ORIGIN_MAX_INSTANCES];
		static int						s_SDKInstanceCount;
    };

    enumContentState ConvertContentState(lsx::ContentStateT state);

}; // namespace Origin

#endif //__ORIGINSDK_IMPL_H__
