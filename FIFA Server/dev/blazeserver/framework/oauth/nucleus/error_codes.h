/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_NUCLEUS_ERRORCODES_H
#define BLAZE_NUCLEUS_ERRORCODES_H

/*** Include files *******************************************************************************/
#include "EASTL/map.h"
#include <string.h>

#include "framework/rpc/oauth_defines.h"
#include "framework/tdf/nucleuscodes.h"
#include "nucleusidentity/tdf/nucleusidentity.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Nucleus
{

class NucleusResult;

class LessString : eastl::binary_function<char*, char*, bool>
{
public:
    bool operator() (const char* str1, const char* str2) const 
    {
        return (strcmp(str1, str2) < 0); 
    }
};

class LessNucleusCode : eastl::binary_function<NucleusCode::Code, NucleusCode::Code, bool>
{
public:
    bool operator() (NucleusCode::Code code1, NucleusCode::Code code2) const 
    {
        return (code1 < code2);
    }
};

class LessNucleusField : eastl::binary_function<NucleusField::Code, NucleusField::Code, bool>
{
public:
    bool operator() (NucleusField::Code code1, NucleusField::Code code2) const 
    {
        return (code1 < code2);
    }
};

class LessNucleusCause : eastl::binary_function<NucleusCause::Code, NucleusCause::Code, bool>
{
public:
    bool operator() (NucleusCause::Code code1, NucleusCause::Code code2) const 
    {
        return (code1 < code2);
    }
};

typedef eastl::map<NucleusCause::Code, BlazeRpcError, LessNucleusCause> ErrorsByCause;
typedef eastl::map<NucleusField::Code, ErrorsByCause*, LessNucleusField> ErrorsByField;
typedef eastl::map<NucleusCode::Code, ErrorsByField*, LessNucleusCode> ErrorsByErrorCode;
typedef eastl::map<int16_t, ErrorsByErrorCode*> ErrorMap;

class ErrorCodes
{
public:
    ErrorCodes()
        : mGenericError(ERR_SYSTEM),
        errors(&mErrorMap)
    {
    }

    ErrorCodes(bool doAddDefaultMapping)
        : mGenericError(ERR_SYSTEM),
        errors(&mErrorMap)
    {
        if (doAddDefaultMapping)
            addDefaultMapping();
    }

    ~ErrorCodes()
    {
        ErrorMap::iterator errorMapIterator = errors->begin();
        while (errorMapIterator != errors->end())
        {
            ErrorsByErrorCode::iterator errorCodeIterator = errorMapIterator->second->begin();
            while (errorCodeIterator != errorMapIterator->second->end())
            {
                ErrorsByField::iterator errorFieldIterator = errorCodeIterator->second->begin();
                while (errorFieldIterator != errorCodeIterator->second->end())
                {
                    ErrorsByCause::iterator errorCauseIterator = errorFieldIterator->second->begin();
                    while (errorCauseIterator != errorFieldIterator->second->end())
                    {
                        errorCauseIterator++;
                    }
                    delete(errorFieldIterator->second);
                    errorFieldIterator++;
                }
                delete(errorCodeIterator->second);
                errorCodeIterator++;
            }
            delete(errorMapIterator->second);
            errorMapIterator++;
        }
        // delete (errors);
    }

    void add(int16_t httpErrorCode, NucleusCode::Code error, NucleusField::Code field, NucleusCause::Code cause, BlazeRpcError errorCode);
    void add(int16_t httpErrorCode, NucleusCode::Code error, NucleusField::Code field, BlazeRpcError errorCode);
    void add(int16_t httpErrorCode, NucleusCode::Code error, BlazeRpcError errorCode);
    void add(int16_t httpErrorCode, BlazeRpcError errorCode);
    
    BlazeRpcError getError(int16_t httpErrorCode, NucleusCode::Code error, NucleusField::Code field, NucleusCause::Code cause) const;
    BlazeRpcError getError(NucleusCode::Code error, NucleusField::Code field, NucleusCause::Code cause) const;
    BlazeRpcError getError(NucleusCode::Code error) const;
    BlazeRpcError getError(const NucleusIdentity::ErrorDetails &error) const;

    void addDefaultMapping()
    {

        // Commerce 2.0 PUT Code Errors:
        add(502, NucleusCode::BAD_GATEWAY, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_BAD_GATEWAY);
        add(404, NucleusCode::INVALID_CODE, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_CODE);
        add(404, NucleusCode::CODE_ALREADY_USED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_CODE_ALREADY_USED);
        add(404, NucleusCode::CODE_ALREADY_DISABLED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_CODE_ALREADY_DISABLED);
        add(404, NucleusCode::NO_ASSOCIATED_PRODUCT, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_ASSOCIATED_PRODUCT);
        add(404, NucleusCode::NO_SUCH_USER, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_USER);
        add(404, NucleusCode::GROUP_NAME_DOES_NOT_MATCH, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_GROUP_NAME_DOES_NOT_MATCH);
        add(404, NucleusCode::NO_SUCH_GROUP, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_SUCH_GROUP);
        add(404, NucleusCode::NO_SUCH_PERSONA, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_PERSONA_NOT_FOUND);
        add(404, NucleusCode::ENTITLEMENT_TAG_MISSING, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_ENTITLEMENT_TAG_MISSING);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_FIELD);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::INVALID_PRODUCT_CONFIGURATION, OAUTH_ERR_INVALID_PRODUCT_CONFIGURATION);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::MULTIPLE_WALLET_ACCOUNTS_FOUND, OAUTH_ERR_MULTIPLE_WALLET_ACCOUNTS_FOUND);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::RESTRICTION_VIOLATION, OAUTH_ERR_RESTRICTION_VIOLATION);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::pidId, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_FIELD);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::pidId, NucleusCause::MISSING_VALUE, OAUTH_ERR_FIELD_MISSING);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::pidId, NucleusCause::INVALID_VALUE, OAUTH_ERR_INVALID_FIELD);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::pidId, NucleusCause::USER_DOES_NOT_MATCH_PERSONA, OAUTH_ERR_USER_DOES_NOT_MATCH_PERSONA);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::productId, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_FIELD);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::productId, NucleusCause::INVALID_PRODUCT_MAPPING, OAUTH_ERR_INVALID_FIELD);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::productId, NucleusCause::PRODUCT_ID_NOT_SPECIFIED, OAUTH_ERR_FIELD_MISSING);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::personaId, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_FIELD);
        add(409, NucleusCode::VALIDATION_FAILED, NucleusField::personaId, NucleusCause::MISSING_VALUE, OAUTH_ERR_FIELD_MISSING);

        add(int16_t(-1), NucleusCode::NO_SUCH_GROUP, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_SUCH_GROUP);
        add(int16_t(-1), NucleusCode::NO_SUCH_GROUP_NAME, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_SUCH_GROUP_NAME);
        add(int16_t(-1), NucleusCode::GROUP_NAME_DOES_NOT_MATCH, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_GROUP_NAME_DOES_NOT_MATCH);
        add(int16_t(-1), NucleusCode::NO_ASSOCIATED_PRODUCT, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_ASSOCIATED_PRODUCT);
        add(int16_t(-1), NucleusCode::CODE_ALREADY_DISABLED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_CODE_ALREADY_DISABLED);
        add(int16_t(-1), NucleusCode::CODE_ALREADY_USED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_CODE_ALREADY_USED);
        add(int16_t(-1), NucleusCode::INVALID_CODE, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, Blaze::OAUTH_ERR_INVALID_CODE);
        add(int16_t(-1), NucleusCode::PARSE_EXCEPTION, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, Blaze::OAUTH_ERR_INVALID_CODE);
        add(int16_t(-1), NucleusCode::UNKNOWN, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, Blaze::ERR_SYSTEM);
        add(int16_t(-1), NucleusCode::NO_SUCH_USER, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_USER);
        add(int16_t(-1), NucleusCode::NO_SUCH_COUNTRY, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_COUNTRY);
        add(int16_t(-1), NucleusCode::NO_SUCH_PERSONA, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_PERSONA_NOT_FOUND);
        add(int16_t(-1), NucleusCode::INVALID_PASSWORD, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_PASSWORD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::PASSWORD, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_PASSWORD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::EMAIL, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_EMAIL);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::PARENTAL_EMAIL, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_PMAIL);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::PARENTAL_EMAIL, NucleusCause::MISSING_VALUE, OAUTH_ERR_INVALID_PMAIL);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::EMAIL, NucleusCause::DUPLICATE_VALUE, OAUTH_ERR_EXISTS);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DISPLAY_NAME, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_PERSONA);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DISPLAY_NAME, NucleusCause::DUPLICATE_VALUE, OAUTH_ERR_EXISTS);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DISPLAY_NAME, NucleusCause::TOO_SHORT, OAUTH_ERR_FIELD_TOO_SHORT);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DISPLAY_NAME, NucleusCause::NOT_ALLOWED, OAUTH_ERR_FIELD_NOT_ALLOWED);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DISPLAY_NAME, NucleusCause::MISSING_VALUE, OAUTH_ERR_FIELD_MISSING);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::STATUS, NucleusCause::ILLEGAL_VALUE, OAUTH_ERR_EXISTS);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DOB, NucleusCause::TOO_YOUNG, OAUTH_ERR_TOO_YOUNG);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::DOB, NucleusCause::INVALID_VALUE, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::TOKEN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_TOKEN);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::EXPIRATION, NucleusCause::UNKNOWN, OAUTH_ERR_EXPIRED_TOKEN);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::TOKEN_EXPIRED, OAUTH_ERR_EXPIRED_TOKEN);
        add(int16_t(-1), NucleusCode::NO_SUCH_EXTERNAL_REFERENCE, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_USER);
        add(int16_t(-1), NucleusCode::NO_SUCH_NAMESPACE, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_NAMESPACE);
        add(int16_t(-1), NucleusCode::NO_SUCH_OPTIN, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_SUCH_OPTIN);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::OPTINTYPE, NucleusCause::INVALID_VALUE, OAUTH_ERR_INVALID_OPTIN);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::OPTINTYPE, NucleusCause::OPTIN_MISMATCH, OAUTH_ERR_OPTIN_MISMATCH);
        add(int16_t(-1), NucleusCode::NO_SUCH_PERSONA_REFERENCE, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_SUCH_PERSONA_REFERENCE);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::SOURCE, NucleusCause::INVALID_VALUE, OAUTH_ERR_INVALID_SOURCE);
        add(int16_t(-1), NucleusCode::NO_SUCH_AUTH_DATA, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_SUCH_AUTH_DATA);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::EMAIL, NucleusCause::NOT_ALLOWED, OAUTH_ERR_TOO_YOUNG);
        add(int16_t(-1), NucleusCode::XBLTOKEN_NOT_FOUND, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_NO_XBLTOKEN);
        add(int16_t(-1), NucleusCode::SANBOXID_NOT_SUPPORTED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_SANDBOX_ID);
        add(int16_t(-1), NucleusCode::XBOX_LOGIN_REQUIRED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_EXPIRED_1PTOKEN);
        add(int16_t(-1), NucleusCode::NO_SUCH_PID, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_INVALID_USER);
        add(int16_t(-1), NucleusCode::SONY_LOGIN_REQUIRED, NucleusField::UNKNOWN, NucleusCause::UNKNOWN, OAUTH_ERR_EXPIRED_1PTOKEN);

        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NO_SUCH_PID, OAUTH_ERR_INVALID_USER);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::PID_DOES_NOT_MATCH, OAUTH_ERR_USER_DOES_NOT_MATCH_PERSONA);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NO_SUCH_PERSONA, OAUTH_ERR_PERSONA_NOT_FOUND);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NO_SUCH_USER, OAUTH_ERR_INVALID_USER);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NO_SUCH_GROUP, OAUTH_ERR_NO_SUCH_GROUP);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::INVALID_CLIENT_GROUP, OAUTH_ERR_GROUPNAME_INVALID);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NO_SUCH_DEVICE, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NO_SUCH_ENTITLEMENT, OAUTH_ERR_NO_SUCH_ENTITLEMENT);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::INVALID_VALUE, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::INVALID_CLIENT_NAMESPACE, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::DUPLICATE_VALUE, OAUTH_ERR_FIELD_ALREADY_EXISTS);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::NOT_ALLOWED, OAUTH_ERR_FIELD_NOT_ALLOWED);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::INVALID_TYPE_FOR_CATEGORY, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::LINK_ALREADY_EXISTS, OAUTH_ERR_FIELD_ALREADY_EXISTS);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::INVALID_DATE_RANGE, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::STATUS, NucleusCause::DATE_STATUS_MISMATCH, OAUTH_ERR_TRIAL_PERIOD_CLOSED);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::TERMINATION_DATE_BEFORE_GRANT_DATE, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::TERMINATION_DATE_RANGE_INVALID, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::GRANT_DATE_RANGE_INVALID, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::RESULT_SET_TOO_LARGE, OAUTH_ERR_TOO_MANY_ENTITLEMENTS);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::OPTIN_MISMATCH, OAUTH_ERR_OPTIN_MISMATCH);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::OPTINTYPE, NucleusCause::NO_SUCH_OPTIN, OAUTH_ERR_NO_SUCH_OPTIN);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::USER_DOES_NOT_MATCH, OAUTH_ERR_INVALID_FIELD);
        add(int16_t(-1), NucleusCode::VALIDATION_FAILED, NucleusField::UNKNOWN, NucleusCause::STALE_VERSION, OAUTH_ERR_INVALID_FIELD);
    }

private:
    ErrorMap mErrorMap;
    BlazeRpcError mGenericError;
    ErrorMap* errors;

    ErrorsByErrorCode* find(ErrorMap* map, int16_t key) const;
    ErrorsByField* find(ErrorsByErrorCode* map, NucleusCode::Code key) const;
    ErrorsByCause* find(ErrorsByField* map, NucleusField::Code key) const;
    BlazeRpcError find(ErrorsByCause* map, NucleusCause::Code key) const;
    
    ErrorsByErrorCode* getErrorsByKey(ErrorMap* errorMap, int16_t key, int16_t defaultKey) const;
    ErrorsByField* getErrorsByKey(ErrorsByErrorCode* errorMap, NucleusCode::Code key, NucleusCode::Code defaultKey) const;
    ErrorsByCause* getErrorsByKey(ErrorsByField* errorMap, NucleusField::Code key, NucleusField::Code defaultKey) const;
    BlazeRpcError getErrorsByKey(ErrorsByCause* errorMap, NucleusCause::Code key, NucleusCause::Code defaultKey) const;
};

} //Nucleus
} //Blaze
#endif /*BLAZE_NUCLEUS_ERRORCODES_H*/
