#ifndef BLAZE_AUTHENTICATION_AUTHENTICATIONIMPL_H
#define BLAZE_AUTHENTICATION_AUTHENTICATIONIMPL_H

#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/metrics/metrics.h"
#include "framework/tdf/oauth_server.h"

// Authentication
#include "authentication/tdf/nucleuscodes.h"
#include "authentication/rpc/authenticationslave_stub.h"
#include "authentication/tdf/authentication.h"
#include "authentication/tdf/authentication_server.h"

#include "nucleusconnect/tdf/nucleusconnect.h"

namespace Blaze
{
// Forward declarations
class InetAddress;

namespace Metrics
{
    using ProjectId = const char8_t*;
    using OnlineAccessEntitlementGroup = const char8_t*;

    namespace Tag
    {
        extern TagInfo<Authentication::EntitlementCheckType>* entitlement_check_type;
        extern TagInfo<ProjectId>* project_id;
        extern TagInfo<OnlineAccessEntitlementGroup>* online_access_entitlement_group;
    }
}

namespace OAuth
{
class OAuthSlaveImpl;
}
namespace Authentication
{

class CheckEntitlementUtil;
class NucleusResult;

enum NucleusBypassType
{
    BYPASS_NONE = 0,
    BYPASS_DEDICATED_SERVER_CLIENT_TYPE,
    BYPASS_ALL
};

struct LegalDocContentInfo
{
    LegalDocContentInfo()
        :mLanguage(nullptr),
        mCountry(nullptr),
        mClientPlatformType(INVALID),
        mLegalDocType(nullptr),
        mContentType(nullptr),
        mLegalDocContentSize(0),
        mLegalDocContentLastTime(0),
        mIsStale(true){}

    ~LegalDocContentInfo() 
    {
        if (mLanguage != nullptr)
        {
            BLAZE_FREE(mLanguage);
            mLanguage = nullptr;
        }
        if (mCountry != nullptr)
        {
            BLAZE_FREE(mCountry);
            mCountry = nullptr;
        }
        if (mLegalDocType != nullptr)
        {
            BLAZE_FREE(mLegalDocType);
            mLegalDocType = nullptr;
        }
        if (mContentType != nullptr)
        {
            BLAZE_FREE(mContentType);
            mContentType = nullptr;
        }
    }

    bool initialize(const char8_t *language,
        const char8_t *country,
        const ClientPlatformType clientPlatformType,
        const char8_t *legalDocType,
        const char8_t *contentType,
        uint32_t legalDocContentSize,
        uint32_t legalDocContentLastTime,
        const char8_t *legalDocUri,
        const char8_t *legalDocContent)
    {
        if (mLanguage != nullptr || mCountry != nullptr || clientPlatformType == INVALID || mLegalDocType != nullptr || 
            mContentType != nullptr || !mLegalDocUri.empty() || !mLegalDocContent.empty() )
        {
            return false;
        }

        if (language != nullptr)
        {
            mLanguage = blaze_strdup(language);
        }

        if (country != nullptr)
        {
            mCountry = blaze_strdup(country);
        }

        mClientPlatformType = clientPlatformType;

        if (legalDocType != nullptr)
        {
            mLegalDocType = blaze_strdup(legalDocType);
        }

        if (contentType != nullptr)
        {
            mContentType = blaze_strdup(contentType);
        }

        mLegalDocContentSize = legalDocContentSize;
        mLegalDocContentLastTime = legalDocContentLastTime;

        if (legalDocUri != nullptr)
        {
            mLegalDocUri = legalDocUri;
        }

        if (legalDocContent != nullptr)
        {
            mLegalDocContent = legalDocContent;
            mIsStale = false;
        }
        else
        {
            mIsStale = true;
        }

        return true;
    }

    void set(uint32_t legalDocContentSize,
        uint32_t legalDocContentLastTime,
        const char8_t *legalDocUri,
        const char8_t *legalDocContent)
    {
        mLegalDocContentSize = legalDocContentSize;
        mLegalDocContentLastTime = legalDocContentLastTime;

        if (legalDocUri == nullptr)
        {
            mLegalDocUri.clear();
        }
        else if(blaze_strcmp(legalDocUri, mLegalDocUri.c_str()) != 0)
        {
            mLegalDocUri = legalDocUri;
        }

        if (legalDocContent == nullptr)
        {
            mLegalDocContent.clear();
        }
        else if(blaze_strcmp(legalDocContent, mLegalDocContent.c_str()) != 0)
        {
            mLegalDocContent = legalDocContent;
        }

        // get latest content and clear the stale flag.
        mIsStale = false;
    }

    const char8_t *getLanguage() const { return mLanguage; }
    const char8_t *getCountry() const { return mCountry; }
    const ClientPlatformType getClientPlatformType() const { return mClientPlatformType; }
    const char8_t *getLegalDocType() const { return mLegalDocType; }
    const char8_t *getContentType() const { return mContentType; }
    uint32_t getLegalDocContentSize() const { return mLegalDocContentSize; }
    uint32_t getLegalDocContentLastTime() const { return mLegalDocContentLastTime; }
    void setLegalDocContentLastTime(uint32_t time) { mLegalDocContentLastTime = time; }
    const eastl::string& getLegalDocUri() const { return mLegalDocUri; }
    const eastl::string& getLegalDocContent() const { return mLegalDocContent; }
    bool getIsStale() const { return mIsStale; }
    void setIsStale(bool stale) { mIsStale = stale; }

    // If the uri or content is nullptr, the legal doc is invalid
    bool isValid() const { return ((!mLegalDocUri.empty()) && (!mLegalDocContent.empty())); }

private:
    char8_t *mLanguage;
    char8_t *mCountry;
    ClientPlatformType mClientPlatformType;
    char8_t *mLegalDocType;
    char8_t *mContentType;
    uint32_t mLegalDocContentSize;
    uint32_t mLegalDocContentLastTime;
    eastl::string mLegalDocUri;
    eastl::string mLegalDocContent;
    bool mIsStale;
};

// struct used to track the metrics information for authentication component
struct AuthenticationMetrics
{
public:
    AuthenticationMetrics(Metrics::MetricsCollection& collection)
        : mLogins(collection, "logins", Metrics::Tag::product_name, Metrics::Tag::client_type)
        , mGuestLogins(collection, "guestLogins", Metrics::Tag::product_name, Metrics::Tag::client_type)
        , mLoginFailures(collection, "loginFailures", Metrics::Tag::product_name, Metrics::Tag::client_type)
        , mLogouts(collection, "logouts", Metrics::Tag::product_name, Metrics::Tag::client_type)
        , mForcedLogouts(collection, "forcedLogouts", Metrics::Tag::product_name, Metrics::Tag::client_type)
        , mSucceedCreatedAccounts(collection, "succeedCreatedAccounts")
        , mFailedCreatedAccountsErrorUnknown(collection, "failedCreatedAccountsErrorUnknown")
        , mSucceedKeyConsumption(collection, "succeedCDKeysConsumptions")
        , mInvalidKeyConsumption(collection, "invalidCDKeysConsumptions")
        , mUsedKeyConsumption(collection, "alreadyUsedCDKeysConsumptions")
        , mUnexpectedKeyConsumption(collection, "otherCDKeyConsumptionError")
        , mAlreadyUsedCodeConsumptions(collection, "alreadyUsedCodeConsumptions")
        , mInvalidCodeConsumptions(collection, "invalidCodeConsumptions")
        , mSucceedCodeConsumptions(collection, "succeedCodeConsumptions")
        , mOtherConsumptionError(collection, "otherConsumptionError")
        , mPsuCutoff(collection, "PSUCutoff", Metrics::Tag::product_name, Metrics::Tag::entitlement_check_type, Metrics::Tag::project_id, Metrics::Tag::online_access_entitlement_group)
        , mSandboxSuccesses(collection, "sandboxLoginSuccess", Metrics::Tag::product_name, Metrics::Tag::sandbox)
        , mSandboxFailures(collection, "sandboxLoginFailure", Metrics::Tag::product_name, Metrics::Tag::sandbox)
    {
        mGaugeLastAuthenticated[0] = 0;
    }

    void getStatusInfo(ComponentStatus& status) const;
    void setLastAuthenticated(const char8_t* lastAuth);

    static void addPerProductStatusInfo(const char8_t* metricName, ComponentStatus& status, const Metrics::TagPairList& tagList, uint64_t value);


public:
    Metrics::TaggedCounter<Metrics::ProductName, ClientType> mLogins;
    Metrics::TaggedCounter<Metrics::ProductName, ClientType> mGuestLogins;
    Metrics::TaggedCounter<Metrics::ProductName, ClientType> mLoginFailures;
    Metrics::TaggedCounter<Metrics::ProductName, ClientType> mLogouts;
    Metrics::TaggedCounter<Metrics::ProductName, ClientType> mForcedLogouts;

    Metrics::Counter mSucceedCreatedAccounts;
    Metrics::Counter mFailedCreatedAccountsErrorUnknown;

    Metrics::Counter mSucceedKeyConsumption;
    Metrics::Counter mInvalidKeyConsumption;
    Metrics::Counter mUsedKeyConsumption;
    Metrics::Counter mUnexpectedKeyConsumption;
    Metrics::Counter mAlreadyUsedCodeConsumptions;
    Metrics::Counter mInvalidCodeConsumptions;
    Metrics::Counter mSucceedCodeConsumptions;
    Metrics::Counter mOtherConsumptionError;
    char8_t mGaugeLastAuthenticated[32];

    Metrics::TaggedGauge<Metrics::ProductName, EntitlementCheckType, Metrics::ProjectId, Metrics::OnlineAccessEntitlementGroup> mPsuCutoff;

    // Sandbox ids used during login
    Metrics::TaggedCounter<Metrics::ProductName, Metrics::Sandbox> mSandboxSuccesses;
    Metrics::TaggedCounter<Metrics::ProductName, Metrics::Sandbox> mSandboxFailures;

};

typedef eastl::intrusive_ptr<OutboundHttpConnectionManager> HttpConnectionManagerPtr;
typedef eastl::vector<HttpConnectionManagerPtr> HttpConnectionManagerList;

class AuthenticationSlaveImpl : public AuthenticationSlaveStub,
    private LocalUserSessionSubscriber
{
public:
    AuthenticationSlaveImpl();
    ~AuthenticationSlaveImpl() override;

    enum ConnectionManagerType
    {
        TOS,
        MAX
    };

    HttpConnectionManagerPtr getConnectionManagerPtr(ConnectionManagerType type) const;

    bool getAllowStressLogin() const;
    bool getAutoEntitlementEnabled(const char8_t* productName) const ;
    bool getAllowUnderage(const char8_t* productName) const;
    bool getNeedCheckEntitlement(ClientType ctype, EntitlementCheckType entType, const char8_t* productName, bool& softCheck) const;
    const char8_t* getProductId(const char8_t* productName) const;
    const char8_t* getOnlineAccessEntitlementGroup(const char8_t* productName) const;
    const char8_t* getProjectId(const char8_t* productName) const;
    const TrialContentIdList* getTrialContentIdList(const char8_t* productName) const;
    const char8_t* getRegistrationSource(const char8_t* productName) const;
    const char8_t* getAuthenticationSource(const char8_t* productName) const;
    const char8_t* getNucleusSourceHeader(const char8_t* productName) const;
    const char8_t* getEntitlementSource(const char8_t* productName) const;
    const char8_t* getEntitlementTag(const char8_t* productName) const; 
    const UnderageLoginOverride* getUnderageLoginOverride(const char8_t* productName) const;
    const char8_t* getLegalDocServer() const;
    const char8_t* getLegalDocGameIdentifier() const;
    ClientPlatformType getPlatform(const Command& callingCommand) const;
    ClientPlatformType getProductPlatform(const char8_t* productName) const;
    static const char8_t* getTOSPlatformStringFromClientPlatformType(const ClientPlatformType clientPlatformType, const char8_t* contentType);
    const AccessList* getAccessList(const char8_t* productName) const;
    const char8_t* getBlazeServerNucleusClientId() const;
    bool passClientGrantWhitelist(const char8_t* productName, const char8_t* groupName) const;
    bool passClientModifyWhitelist(const char8_t* productName, const char8_t* groupName) const;
    bool shouldBypassRateLimiter(ClientType clientType) const;
    bool isBelowPsuLimitForClientType(ClientType ClientType) const;
    uint32_t getPsuLimitForClientType(ClientType clientType) const;

    bool getEnableBypassForDedicatedServers() const;
    bool areExpressCommandsDisabled(const char8_t* context, const PeerInfo* peerInfo, const char8_t* username) const;

    static BlazeRpcError getError(const NucleusResult* result);
    static Blaze::OAuth::OAuthSlaveImpl* oAuthSlaveImpl(); // Local oAuthSlaveImpl - Transition helper

    // common command code
    BlazeRpcError DEFINE_ASYNC_RET(commonEntitlementCheck(
        AccountId accountId,
        CheckEntitlementUtil& checkEntitlementUtil,
        ClientType clientType,
        const char8_t* productName,
        const Command* callingCommand = nullptr,
        const char8_t* accessToken = nullptr,
        const char8_t* clientId = nullptr,
        bool skipAutoGrantEntitlement = false));

    static BlazeRpcError checkListEntitlementsRequest(const GroupNameList& groupNameList, const uint16_t pageSize, const uint16_t pageNo);
    static BlazeRpcError isValidGroupNameList(const GroupNameList& groupNameList);
    static BlazeRpcError checkListEntitlementsPageSizeAndNumber(const uint16_t pageSize, const uint16_t pageNo);

    BlazeRpcError verifyConsoleEnv(ClientPlatformType platform, const char8_t *productName, const char8_t *consoleEnv);

    bool isTrustedSource(const Command* callingCommand) const;

    bool isValidProductName(const char8_t* productName) const;

    // Status info
    void getStatusInfo(ComponentStatus& status) const override;

    AuthenticationMetrics* getMetricsInfoObj() { return &mMetricsInfo; }

    BlazeRpcError DEFINE_ASYNC_RET( tickAndCheckLoginRateLimit(bool doDelay = true));

    void setBypassNucleus(bool noNucleus) { mBypassNucleusEnabled = noNucleus; }

    const AuthenticationConfig::NucleusBypassURIList& getNucleusBypassList() const { return getConfig().getNucleusBypass(); }
    NucleusBypassType isBypassNucleusEnabled(ClientType clientType) const;

    const LegalDocContentInfo *getLastestLegalDocContentInfoByContentType(const char8_t* isolanguage, const char8_t *isoCountry, ClientPlatformType clientPlatformType, 
        const char8_t *docType, GetLegalDocContentRequest::ContentType contentType, const Command* callingCommand = nullptr);

    const LegalDocContentInfo *getLastestLegalDocContentInfoAnyContentType(const char8_t* language, const char8_t *country, ClientPlatformType clientPlatformType, 
        const char8_t *docType, bool acceptExpired = false, bool delayUpdate = false, const Command* callingCommand = nullptr);

    const LegalDocContentInfo *getLegalDocContentInfo(const char8_t* legalDocKey) const 
    {
        LegalDocContentInfoMap::const_iterator itr = mLegalDocContentInfoMap.find(legalDocKey);
        return itr != mLegalDocContentInfoMap.end() ? &(itr->second) : nullptr; 
    }

    void obfuscatePlatformInfo(ClientPlatformType platform, EA::TDF::Tdf& tdf) const override;

private:
    BlazeRpcError DEFINE_ASYNC_RET(updateLegalDocContent(const char8_t *language, const char8_t *country, const ClientPlatformType clientPlatformType,
        const char8_t *docType, const char8_t *contentType, bool delayUpdate = false, const Command* callingCommand = nullptr));
    void convertLegalDocKey(const char8_t* language, const char8_t *country, const ClientPlatformType clientPlatformType,
        const char8_t *docType, const char8_t* contentType, char8_t *legalDocKey, size_t length = LEGAL_DOC_KEY_MAX_LENGTH) const;
    void scheduleNextCheckLegalDoc();
    void checkLegalDocMap();
    bool isLegalDocStale(const LegalDocContentInfo* curContentInfo, const Command* callingCommand = nullptr);

    bool onConfigure() override;
    bool onReconfigure() override;
    bool configure();
    bool onResolve() override;
    void onShutdown() override;
    bool onActivate() override;

    // event handlers for User Session Manager events
    void onLocalUserSessionLogout(const UserSessionMaster& userSession) override;

    bool onValidateConfig(AuthenticationConfig& config, const AuthenticationConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    void setProductEntitlements();
    void clearProductEntitlements();
    BlazeRpcError DEFINE_ASYNC_RET(checkEntitlement(EntitlementCheckType type, AccountId accountId, CheckEntitlementUtil& checkEntitlementUtil, EntitlementStatus::Code& entitlementStatus,
        const char8_t* productName, uint32_t& psuCutOff, bool autoGrantEnabled, const Command* callingCommand = nullptr, const char8_t* accessToken = nullptr));
    BlazeRpcError postEntitlement(EntitlementCheckType type, AccountId accountId, EntitlementStatus::Code& entitlementStatus, const char8_t* productName,
        uint32_t& psuCutOff, const Command* callingCommand = nullptr, const char8_t* accessToken = nullptr, const char8_t* clientId = nullptr);
    bool isAutoGrantEnabled(EntitlementCheckType type, const char8_t* productName) const;

    void entitlementStatusCheck();

    void clearPsuCutoffProductMetrics();

private:

    bool mBypassNucleusEnabled;

    float_t mGlobalLoginRateLimit;
    uint32_t mGlobalLoginRateLimitCounter;
    TimeValue mCurrentLoginRateInterval;

    struct LoginIntervalInfo
    {
        uint32_t logins;
        TimeValue interval;
    };
    typedef eastl::list<LoginIntervalInfo> LoginIntervalInfoList;
    LoginIntervalInfoList mLoginIntervalInfoList;

    bool mUseEntitlementWhitelist;

    //key is Language_Country_Platform_DocType_ContentType, like as en_us_pc_webterms_plain
    typedef eastl::hash_map<eastl::string, LegalDocContentInfo> LegalDocContentInfoMap;
    LegalDocContentInfoMap mLegalDocContentInfoMap;

    // Redefine some of the TDF types here as intrusive_ptrs
    // so that we don't invalidate iterators that could be in use during a reconfigure
    typedef EA::TDF::tdf_ptr<AccessList> AccessListPtr;
    typedef eastl::map<EntitlementCheckType, AccessListPtr> EntitlementCheckMap;
    typedef eastl::map<ProductName, EntitlementCheckMap, EA::TDF::TdfStringCompareIgnoreCase> ProductEntitlementMap;
    ProductEntitlementMap mProductEntitlementMap;

    static const size_t LEGAL_DOC_KEY_MAX_LENGTH = 256;

    TimerId mCheckLegalDocId;
    bool mStopCheckLegalDoc;
    bool mTosServiceDown;

    AuthenticationMetrics mMetricsInfo;
    InetFilter mTrustedFilter;

    TimerId mEntitlementStatusCheckTimerId;
    bool mEntitlementStatusCheckInProgress;

    FiberJobQueue mBackgroundJobQueue;

    static const char8_t* sPoolNames[ConnectionManagerType::MAX];
};

} // Authentication
} // Blaze
#endif
