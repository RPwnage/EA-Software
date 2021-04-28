/**************************************************************************************************/
/*! 
    \file logindata.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOGINDATA_H
#define LOGINDATA_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/component/authentication/tdf/accountdefines.h"
#include "BlazeSDK/component/authentication/tdf/authentication.h"
#include "BlazeSDK/component/authenticationcomponent.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

namespace Blaze
{

class BlazeHub;

namespace LoginManager
{

enum LoginFlowType
{
    LOGIN_FLOW_NONE,
    LOGIN_FLOW_STANDARD,
    LOGIN_FLOW_SILENT,
    LOGIN_FLOW_ORIGIN
};

/*! ***********************************************************************************************/
/*! \class LoginData
    
    \brief Stores data shared by the login manager and the login state machine for the duration of
           the login process.
***************************************************************************************************/
class LoginData
{
    friend class LoginManagerImpl;
    friend class LoginStateBase;
    friend class LoginStateInit;
    friend class LoginStateInitS2S;
    friend class LoginStateInitConsole;

public:
    LoginData(MemoryGroupId memGroupId = MEM_GROUP_LOGINMANAGER_TEMP)
    :   mLoginFlowType(LOGIN_FLOW_NONE),
        mPersonaDetailsList(memGroupId, MEM_NAME(memGroupId, "LoginData::mPersonaDetailsList")),
        mAccountId(INVALID_ACCOUNT_ID),
        mTermsOfService(nullptr),
        mPrivacyPolicy(nullptr),
        mLastLoginError(ERR_OK),
        mIsOfLegalContactAge(false),
        mIsUnderage(false),
        mIsUnderageSupported(false),
        mThirdPartyOptin(false),
        mGlobalOptin(false),
        mIsAnonymous(false),
        mNeedsLegalDoc(false),
        mSkipLegalDocumentDownload(false)
    {
        mIsoCountryCode[0] = '\0';
        mTermsOfServiceVersion[0] = '\0';
        mPrivacyPolicyVersion[0] = '\0';
        mLoginEmail[0] = '\0';
        mLoginPassword[0] = '\0';
        mLoginPersonaName[0] = '\0';
        mParentEmail[0] = '\0';
        mToken.clear();
    }

    ~LoginData()
    {
        clear();
    }

    LoginFlowType getLoginFlowType() { return mLoginFlowType; }

    const Authentication::PersonaDetailsList& getPersonaDetailsList() { return mPersonaDetailsList; }


    /*! ***************************************************************************/
    /*!    \brief Gets the persona in the persona details list with the matching name 
               and namespace.
    
        \param name Name to match.
        \return The persona details, or nullptr if not found.
    *******************************************************************************/
    const Authentication::PersonaDetails *getPersona(const char8_t* name)
    {
        const Authentication::PersonaDetails *result = nullptr;
        Authentication::PersonaDetailsList::const_iterator i = mPersonaDetailsList.begin();
        Authentication::PersonaDetailsList::const_iterator end = mPersonaDetailsList.end();
        for (; i != end; ++i)
        {
            if (blaze_strcmp((*i)->getDisplayName(), name) == 0)
            {
                result = *i;
            }
        }

        return result;
    }

    AccountId getAccountId() const { return mAccountId; }

    const char8_t* getIsoCountryCode() const { return const_cast<const char8_t*>(mIsoCountryCode); }

    const char8_t* getTermsOfService() const { return mTermsOfService; }

    const char8_t* getPrivacyPolicy() const { return mPrivacyPolicy; }

    const char8_t* getTermsOfServiceVersion() { return mTermsOfServiceVersion; }

    const char8_t* getPrivacyPolicyVersion() { return mPrivacyPolicyVersion; }

    const char8_t* getPassword() { return mLoginPassword;}

    const char8_t* getPersonaName() { return mLoginPersonaName;}

    const char8_t* getToken() { return mToken.c_str();}

    BlazeError getLastLoginError() { return mLastLoginError;}

    uint8_t getIsOfLegalContactAge() { return mIsOfLegalContactAge;}

    void getSpammable(bool& eaEmailAllowed, bool& thirdPartyEmailAllowed)
    {
        eaEmailAllowed = mGlobalOptin;
        thirdPartyEmailAllowed = mThirdPartyOptin;
    }

    bool getNeedsLegalDoc() { return mNeedsLegalDoc; }

    bool getSkipLegalDocumentDownload() const { return mSkipLegalDocumentDownload; }

    bool getIsAnonymous() { return mIsAnonymous; }

    bool getIsUnderage() { return mIsUnderage; }
    bool getIsUnderageSupported() { return mIsUnderageSupported; }

protected:
    void clear()
    {
        mLoginFlowType = LOGIN_FLOW_NONE;
        mAccountId = INVALID_ACCOUNT_ID;
        mIsoCountryCode[0] = '\0';
        mLoginEmail[0] = '\0';
        mParentEmail[0] = '\0';
        mLoginPassword[0] = '\0';
        mLoginPersonaName[0] = '\0';
        mToken.clear();
        mLastLoginError = ERR_OK;
        mIsOfLegalContactAge = false;
        mIsUnderage = false;
        mIsUnderageSupported = false;
        clearLegalDocs();
        mThirdPartyOptin = 0;
        mGlobalOptin = 0;
        mIsAnonymous = false;
        mNeedsLegalDoc = false;
        mSkipLegalDocumentDownload = false;
        mPersonaDetailsList.release();
    }
    
    void clearLegalDocs()
    {
        if (mTermsOfService != nullptr)
        {
            BLAZE_FREE(MEM_GROUP_LOGINMANAGER, mTermsOfService);
            mTermsOfService = nullptr;
        }
        if (mPrivacyPolicy != nullptr)
        {
            BLAZE_FREE(MEM_GROUP_LOGINMANAGER, mPrivacyPolicy);
            mPrivacyPolicy = nullptr;
        }
        mTermsOfServiceVersion[0] = '\0';
        mPrivacyPolicyVersion[0] = '\0';
    }

    void setLoginFlowType(LoginFlowType loginFlowType)
    {
        mLoginFlowType = loginFlowType;
    }

    void setPersonaDetailsList(const Authentication::PersonaDetailsList *list) 
    {
        mPersonaDetailsList.release();
        Authentication::PersonaDetailsList::const_iterator i = list->begin();
        Authentication::PersonaDetailsList::const_iterator end = list->end();
        for (; i != end; ++i)
        {
            Authentication::PersonaDetails* persona = (*i)->clone(MEM_GROUP_LOGINMANAGER);
            mPersonaDetailsList.push_back(persona);
        }
    }

    /*! ***************************************************************************/
    /*!    \brief Adds an individual persona to the list.
        
        The passed persona is copied into the list.
    
        \param persona The persona to add.  
    *******************************************************************************/
    void addPersona(const Authentication::PersonaDetails *persona)
    {
        Authentication::PersonaDetails *newPersona = persona->clone(MEM_GROUP_LOGINMANAGER);
        mPersonaDetailsList.push_back(newPersona);
    }

    /*! ***************************************************************************/
    /*!    \brief Removes an individual persona from the list.
    
        \param persona Persona to remove.
    *******************************************************************************/
    void removePersona(PersonaId id)
    {
        Authentication::PersonaDetailsList::iterator i = mPersonaDetailsList.begin();
        Authentication::PersonaDetailsList::iterator end = mPersonaDetailsList.end();
        for (; i != end; ++i)
        {
            if ((*i)->getPersonaId() == id)
            {
                mPersonaDetailsList.erase(i);
                break;
            }
        }
    }

    void setAccountId(AccountId accountId)
    {
        mAccountId = accountId;
    }

    void setIsoCountryCode(const char8_t* isoCountryCode)
    {
        blaze_strnzcpy(mIsoCountryCode, isoCountryCode, sizeof(mIsoCountryCode));
    }

    void setTermsOfService(char8_t* termsOfService) { mTermsOfService = termsOfService; }

    void setPrivacyPolicy(char8_t* privacyPolicy) { mPrivacyPolicy = privacyPolicy; }

    void setTermsOfServiceVersion(const char8_t* termsOfServiceVersion)
    {
        blaze_strnzcpy(mTermsOfServiceVersion, termsOfServiceVersion, sizeof(mTermsOfServiceVersion));
    }

    void setPrivacyPolicyVersion(const char8_t* privacyPolicyVersion)
    {
        blaze_strnzcpy(mPrivacyPolicyVersion, privacyPolicyVersion, sizeof(mPrivacyPolicyVersion));
    }

    void setPassword(const char8_t* password)
    {
        blaze_strnzcpy(mLoginPassword, password, sizeof(mLoginPassword));
    }

    void setPersonaName(const char8_t* personaName)
    {
        blaze_strnzcpy(mLoginPersonaName, personaName, sizeof(mLoginPersonaName));
    } 

    void setToken(const char8_t* token)
    {
        if (token != nullptr)
            mToken.assign(token);
    }

    void setLastLoginError(BlazeError error)
    {
        mLastLoginError = error;
    }

    void setIsOfLegalContactAge(uint8_t isOfLegalContactAge)
    {
        mIsOfLegalContactAge = isOfLegalContactAge;
    }

    void setSpammable(bool eaEmailAllowed, bool thirdPartyEmailAllowed)
    {
        mGlobalOptin = eaEmailAllowed;
        mThirdPartyOptin = thirdPartyEmailAllowed;
    }

    void setIsUnderage(bool isUnderage) 
    {
        mIsUnderage = isUnderage;
    }

    void setIsUnderageSupported(bool isUnderageSupported) 
    {
        mIsUnderageSupported = isUnderageSupported;
    }

    void setIsAnonymous(bool isAnonymous) 
    {
        mIsAnonymous = isAnonymous;
    }

    void setNeedsLegalDoc(bool needsLegalDoc)
    {
        mNeedsLegalDoc = needsLegalDoc;
    }

    void setSkipLegalDocumentDownload(bool skipLegalDocumentDownload)
    {
        mSkipLegalDocumentDownload = skipLegalDocumentDownload;
    }

protected:
    LoginFlowType                   mLoginFlowType;
    Authentication::PersonaDetailsList mPersonaDetailsList;
    AccountId                       mAccountId;
    char8_t                         mIsoCountryCode[Authentication::MAX_COUNTRY_ISO_CODE_LENGTH];
    char8_t*                        mTermsOfService;
    char8_t*                        mPrivacyPolicy;
    char8_t                         mTermsOfServiceVersion[Authentication::MAX_LEGALDOC_VERSION_LENGTH];
    char8_t                         mPrivacyPolicyVersion[Authentication::MAX_LEGALDOC_VERSION_LENGTH];
    char8_t                         mLoginEmail[MAX_EMAIL_LENGTH];
    char8_t                         mParentEmail[MAX_EMAIL_LENGTH];
    char8_t                         mLoginPassword[Authentication::MAX_PASSWORD_LENGTH];
    char8_t                         mLoginPersonaName[Blaze::MAX_PERSONA_LENGTH];
    Blaze::string                   mToken;
    BlazeError                      mLastLoginError;
    uint8_t                         mIsOfLegalContactAge;
    bool                            mIsUnderage;
    bool                            mIsUnderageSupported;
    bool                            mThirdPartyOptin;
    bool                            mGlobalOptin;
    bool                            mIsAnonymous;
    bool                            mNeedsLegalDoc;
    bool                            mSkipLegalDocumentDownload;
};
} // LoginManager
} // Blaze

#endif // LOGINDATA_H
