// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Hub.h>

namespace eadp
{
namespace foundation
{

/*!
 * @brief Identity service manages the user authentication.
 */
class EADPSDK_API IIdentityService
{
public:

    /*!
     * @brief Slot authentication status
     *
     * Indicate current slot status.
     */
    enum class Status : uint32_t
    {
        UNUSED,         /*!< Slot not in use and may be used for authenticating user */
        AUTHENTICATING, /*!< Service is processing an authentication request for given slot */
        AUTHENTICATED,  /*!< Service is finished an authentication request for given slot */
        REFRESHING,     /*!< Service is refreshing an expiring token or updating cached user info */
        READY,          /*!< Service has completed all pending authentication requests and initialization. Integrator at this point may call getters to read additional information available for authenticated user, like persona/pid info */
    };

    /*!
     * @brief Identity origin
     *
     * Indicate authentication source.
     */
    enum class Version : uint32_t
    {
        IDENTITY_UNKNOWN, /*!< Token source is unknown */
        IDENTITY_2_0,     /*!< Token issued by identity 2.0 service */
        IDENTITY_2_1,     /*!< Token issued by identity 2.1 service */
    };

    /*!
     * @brief Callback to listen status update.
     *
     * @param status New status.
     * @param userIndex Slot for multiple users authentication, which status been updated.
     */
    using StatusUpdateCallback = CallbackT<void(Status status, uint32_t userIndex)>;

    /*!
     * @brief Invalid user index
     *
     * This value normally indicates no identity authentication is needed.
     */
    static const uint32_t kInvalidUserIndex = UINT32_MAX;

    /*!
     * @brief Default user index
     *
     * If not explicitly set, the slot 0 will be used for authentication
     */
    static const uint32_t kDefaultUserIndex = 0;

    /*!
     * @brief Structure representing token information
     */
    struct EADPSDK_API TokenInfo
    {
        /*!
         * @brief Structure containing authenticator information
         */
        struct EADPSDK_API Authenticator
        {
            explicit Authenticator(const Allocator& allocator);
            Authenticator(const Authenticator& other);

            /*!
             * @brief Authenticator type
             */
            String m_type;
            /*!
             * @brief Pid identifier for the authenticator
             */
            String m_pidId;
        };
        
        explicit TokenInfo(const Allocator& allocator);
        TokenInfo(const TokenInfo& other);

        /*!
         * @brief Access token
         */
        String m_accessToken;
        /*!
         * @brief Access token type
         */
        String m_accessTokenType;
        /*!
         * @brief Client id for current token
         */
        String m_clientId;
        /*!
         * @brief Scope for current token
         */
        String m_scope;
        /*!
         * @brief Expiration time for current token
         */
        int64_t m_expirationTime;
        /*!
         * @brief Pid id for current token
         */
        String m_pidId;
        /*!
         * @brief Pid type for current token
         */
        String m_pidType;
        /*!
         * @brief User id for current token
         */
        String m_userId;
        /*!
         * @brief Persona id for current token
         */
        String m_personaId;
        /*!
         * @brief List of logged in authenticators
         */
        Vector<Authenticator> m_authenticators;

    private:
        Allocator m_allocator;
    };

    /*!
     * @brief Structure representing persona information
     */
    struct EADPSDK_API PersonaInfo
    {
        explicit PersonaInfo(const Allocator& allocator);
        PersonaInfo(const PersonaInfo& other);

        /*!
         * @brief Persona id
         */
        String m_personaId;

        /*!
         * @brief Pid id for current persona
         */
        String m_pidId;

        /*!
         * @brief Display name for current persona
         */
        String m_displayName;

        /*!
         * @brief Nick name for current persona
         */
        String m_name;

        /*!
         * @brief Name of name space for current persona
         */
        String m_namespaceName;

        /*!
         * @brief Visibility flag for current persona
         */
        bool m_visible;

        /*!
         * @brief Status for current persona
         */
        String m_status;

        /*!
         * @brief Status reason code for current persona
         */
        String m_statusReasonCode;

        /*!
         * @brief Privacy level for current persona
         */
        String m_privacyLevel;

        /*!
         * @brief Creation date for current persona
         */
        String m_dateCreated;

        /*!
         * @brief Last authentication date for current persona
         */
        String m_lastAuthenticated;

        /*!
         * @brief Authentication source for current persona
         */
        String m_authenticationSource;
    };

    /*!
     * @brief Structure representing certificate information
     */
    struct EADPSDK_API CertificateInfo
    {
        explicit CertificateInfo(const Allocator& allocator);

        /*!
         * @brief The certificate to use for S2S authentication.
         */
        String m_certificate;

        /*!
         * @brief The certificate key to use for S2S authentication.
         */
        String m_certificateKey;
    };

    /*!
     * @brief Get an token information. Token information available when status for selected slot is Status:READY.
     * Status can be verified by calling getStatus(userIndex) or by listening for status updates (see addStatusUpdateCallback).
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A structure containing token info for specified slot.
     */
    virtual TokenInfo getTokenInfo(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Sets token information.
     *
     * Exposed for use by the Authentication module.  Not intended for use by integrators.
     *
     * @param info A structure containing token info for specified slot.
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     */
    virtual void setTokenInfo(const TokenInfo& info, uint32_t userIndex = kDefaultUserIndex) = 0;
    
    /*!
     * @brief Get an persona information. Persona information available when status for selected slot is Status:READY.
     * Status can be verified by calling getStatus(userIndex) or by listening for status updates (see addStatusUpdateCallback).
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A structure containing persona info for specified slot.
     */
    virtual PersonaInfo getPersonaInfo(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Sets persona information.
     *
     * Exposed for use by the Authentication module.  Not intended for use by integrators.
     *
     * @param info A structure containing persona info for specified slot.
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     */
    virtual void setPersonaInfo(const PersonaInfo& info, uint32_t userIndex = kDefaultUserIndex) = 0;
    
    /*!
     * @brief Virtual default destructor for base class
     */
    virtual ~IIdentityService() = default;

    /*!
     * @brief Returns the current status of the service.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return The status of the service for given slot.
     */
    virtual Status getStatus(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Update the current status of the service.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @param status New status of the service for given slot.
     */
    virtual void updateStatus(Status status, uint32_t userIndex = kDefaultUserIndex) = 0;

    /*!
     * @brief Adds a callback for status change notifications.
     *
     * The callback will be invoked whenever the status of the service changes.  To remove the
     * handler, release the persistence object used when creating the callback.
     */
    virtual void addStatusUpdateCallback(const StatusUpdateCallback& callback) = 0;
    
    /*!
     * @brief Indicates if a user is authenticated with the service
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return True if a user in target slot is authenticated.
     */
    virtual bool isAuthenticated(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Gets an Access Token for user.
     *
     * If a user is currently authenticated, this method returns a string containing an access token.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's access token for specified slot.
     */
    virtual String getAccessToken(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Set an Access Token for the user.
     *
     * @param token Nucleus access token
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     */
    virtual void setAccessToken(const char8_t* token, uint32_t userIndex = kDefaultUserIndex) = 0;

    /*!
    * @brief Set an Access Token for the user.
    *
    * Provided for compatibility.  Not intended for use by integrators.
    *
    * @deprecated Use simple setAccessToken method and setTokenInfo.
    *
    * @param token Nucleus access token
    * @param tokenType Nucleus access token type
    * @param tokenIdentityVersion Token identity version. Currently supported ID 2.0 and ID 2.1.
    * Depending on token identity version different nucleus servers provides different information about authenticated user.
    * @param userIndex Slot for multiple users authentication.
    */
    virtual void setAccessToken(const char8_t* token, const char8_t* tokenType, Version tokenIdentityVersion, uint32_t userIndex) = 0;

    /*!
    * @brief Gets an Platform User for userIndex.
    *
    * If a user is currently authenticated, this method returns a IEAUser for the given userIndex.
    *
    * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
    * @return An IEAUser for specified slot.
    */
    virtual User* getUserInfo(uint32_t userIndex) = 0;

    /*!
    * @brief Set an User Info for the user.
    *
    * @param userInfo IEAUser object (platform specific)
    * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
    */
    virtual void setUserInfo(User* user, uint32_t userIndex = kDefaultUserIndex) = 0;

    /*!
     * @brief Get certificate information. Certificate information will be available after a call to
     *        S2SAuthService::authenticate has been made.
     *
     * @return A structure containing certificate information.
     */
    virtual CertificateInfo getCertificateInfo() const = 0;

    /*!
     * @brief Sets certificate information.
     *
     * Exposed for use by the Foundation module.  Not intended for use by integrators.
     *
     * @param info A structure containing certificate info.
     */
    virtual void setCertificateInfo(const CertificateInfo& info) = 0;

    /*!
     * @brief Get an Token Client Id for the user.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's token client id for specified slot.
     */
    virtual String getTokenClientId(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an scope for the user.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's scope for specified slot.
     */
    virtual String getScope(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an expiration time for the token.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A token expiration time in seconds for specified slot.
     */
    virtual int64_t getExpirationTime(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's pid id.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's pid id for specified slot.
     */
    virtual String getPidId(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's pid type.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's pid type for specified slot.
     */
    virtual String getPidType(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user id.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's id for specified slot.
     */
    virtual String getUserId(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an persona id for the user.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's persona id for specified slot.
     */
    virtual String getPersonaId(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's display name.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's display name for current persona id for specified slot.
     */
    virtual String getDisplayName(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's nick name.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's nick name for current persona id for specified slot.
     */
    virtual String getNickName(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an namespace name for current persona id.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's namespace name for current persona id for specified slot.
     */
    virtual String getNamespaceName(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's visibility.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return The user's visibility for current persona id for specified slot.
     */
    virtual bool getPersonaVisibility(uint32_t userIndex = kDefaultUserIndex) = 0;

    /*!
     * @brief Get an user's status.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's status for current persona id for specified slot.
     */
    virtual String getPersonaStatus(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's status reason code.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's status reason code for current persona id for specified slot.
     */
    virtual String getPersonaStatusReasonCode(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's privacy level.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's privacy level for current persona id for specified slot.
     */
    virtual String getPrivacyLevel(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an date of persona creation.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's date of creation for current persona id for specified slot.
     */
    virtual String getDateCreated(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an date of last authenticaton.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's date of authenticaton for current persona id for specified slot.
     */
    virtual String getLastAuthenticated(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Get an user's authenticaton source.
     *
     * @param userIndex Slot for multiple users authentication. Default slot is kDefaultUserIndex.
     * @return A string containing the user's authenticaton source for current persona id for specified slot.
     */
    virtual String getAuthenticationSource(uint32_t userIndex = kDefaultUserIndex) const = 0;

    /*!
     * @brief Request containing information needed to obtain an auth code for an authenticated user.
     */
    class EADPSDK_API GetAuthCodeRequest
    {
    public:
        /*!
         * @brief Constructs a request for retrieving an auth code for an authenticated user.
         */
        explicit GetAuthCodeRequest(const Allocator& allocator);
        
        /*! @{
         * @brief Accessors for setting and retrieving the user index indicating the slot in the user table.
         */
        uint32_t getUserIndex() const;
        void setUserIndex(uint32_t userIndex);
        /*! @} */
        
        /*! @{
         * @brief Accessors for setting and retrieving the client ID for the new auth code.
         */
        const eadp::foundation::String& getClientId() const;
        void setClientId(const eadp::foundation::String& value);
        void setClientId(const char8_t* value);
        /*! @} */

        /*! @{
         * @brief Accessors for setting and retrieving the redirect URI desired for the new auth code.
         *
         * Defaults to 'nucleus:rest' if not provided.
         */
        const eadp::foundation::String& getRedirectUri() const;
        void setRedirectUri(const eadp::foundation::String& value);
        void setRedirectUri(const char8_t* value);
        /*! @} */

        /*! @{
         * @brief Accessors for setting and retrieving the scope desired for the new auth code.
         */
        const eadp::foundation::String& getScope() const;
        void setScope(const eadp::foundation::String& value);
        void setScope(const char8_t* value);
        /*! @} */
        
    private:
        Allocator m_allocator;
        uint32_t m_userIndex;
        String m_clientId;
        String m_redirectUri;
        String m_scope;
    };
    
    /*!
     * @brief Response for delivering an auth code retrieved for an authenticated user.
     */
    class EADPSDK_API GetAuthCodeResponse
    {
    public:
        /*!
         * @brief Constructs a response for delivering an auth code for an authenticated user.
         */
        explicit GetAuthCodeResponse(const Allocator& allocator);
        
        /*! @{
         * @brief Accessors for setting and retrieving the auth code.
         */
        const eadp::foundation::String& getAuthCode() const;
        void setAuthCode(const eadp::foundation::String& value);
        void setAuthCode(const char8_t* value);
        /*! @} */

        /*! @{
         * @brief Accessors for setting and retrieving the user index indicating the slot in the user table.
         */
        uint32_t getUserIndex() const;
        void setUserIndex(uint32_t userIndex);
        /*! @} */

    private:
        Allocator m_allocator;
        String m_authCode;
        uint32_t m_userIndex;
    };

    /*!
     * @brief Callback for delivery of requested Auth Codes.
     */
    using GetAuthCodeCallback = CallbackT<void(UniquePtr<GetAuthCodeResponse> response, const ErrorPtr& error)>;

    /*!
     * @brief Get an Auth code for an authenticated user for a new client ID and scope.
     *
     * @param request Information defining the auth code request parameters.
     */
    virtual void getAuthCode(UniquePtr<GetAuthCodeRequest> request, GetAuthCodeCallback callback) = 0;

protected:
    IIdentityService() = default;

    EA_NON_COPYABLE(IIdentityService);
};

/*!
 * @brief The identity service access class
 *
 * It helps to get semi-singleton identity service instance on Hub, so other service can access easily.
 */
class EADPSDK_API IdentityService
{
public:
    /*!
     * @brief Get the identity service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the logging service.
     * @return The IIdentityService instance
     */
    static IIdentityService* getService(IHub* hub);
};

}
}

