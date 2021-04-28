///////////////////////////////////////////////////////////////////////////////\
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////\
//
//	
#ifndef __EACORESTATUSMAP_H_INCLUDED_ 
#define __EACORESTATUSMAP_H_INCLUDED_

#include "PropertiesMap.h"
#include "CoreCmdPortal_ReqdCmds.h"
#include "CmdPortalFunctionNames.h"
#include "LoginStatus.h"

#include "services/plugin/PluginAPI.h"

class ORIGIN_PLUGIN_API LoginError : public QObject
{
    Q_OBJECT
public:
    ///
    /// Operator assignment
    ///
    virtual LoginError& operator=(const LoginError& sle)
    {
        if (this != &sle)
        {
            set(sle);
        }
        return *this;
    }
    ///
    /// CTOR
    ///
    LoginError(const QString& errorCode, const QString& errorString, Core::LoginStatus loginStatus, const QString& messageString)		
        :mErrorCode(errorCode), mErrorString(errorString), mLoginStatus(loginStatus), mMessageString(messageString)
    {
    }

    ///
    /// Copy CTOR
    ///
    LoginError(const LoginError& sle)
    {
        set(sle);
    }
    ///
    /// Default CTOR
    ///
    LoginError()
    {

    }
    ///
    /// DTOR
    ///
    virtual ~LoginError() {}

    ///
    ///
    ///
    QString eCode() const {return mErrorCode;}
    QString eString() const {return mErrorString;}
    QString mString() const {return mMessageString;}
    Core::LoginStatus loginStatus() const {return mLoginStatus;}

    ///
    /// Retrieves the message error string based on the server error code
    ///
    template <typename T>
    static QString messageString(const QString& errorCode)
    {
        T::createMap();
        QString res;
        if (T::map().count(errorCode) > 0)
        {
            res = T::map()[errorCode].mString();
        }
        return res;
    }
    ///
    /// Retrieves the server error string based on the error code
    ///
    template <typename T>
    static QString errorString(const QString& errorCode)
    {
        T::createMap();
        QString res;
        if (T::map().count(errorCode) > 0)
        {
            res = T::map()[errorCode].eString();
        }
        return res;
    }
    ///
    /// Retrieves the error string based on the login status
    /// Returns an empty QString if the error was not found for asserting purposes
    /// linear time lookup
    ///
    template <typename T>
    static QString errorString(Core::LoginStatus ls)
    {
        T::createMap();
        QString res;
        foreach(T sle, T::map())
        {
            if (sle.loginStatus() == ls)
            {
                res = sle.eString();
                break;
            }
        }
        return res;
    }
    ///
    /// Retrieves the login status code based on the error code
    ///
    template <typename T>
    static Core::LoginStatus loginErrorCodeFromCode(const QString& errorCode)
    {
        T::createMap();
        Core::LoginStatus res = Core::LoginStatusErrorUnknown;
        if (T::map().count(errorCode) > 0)
        {
            res = T::map()[errorCode].loginStatus();
        }
        return res;
    }
    ///
    /// Retrieves the error data in one single object
    ///
    template <typename T>
    static bool loginError(const QString& errorCode,T& sle)
    {
        T::createMap();
        if (T::map().count(errorCode) > 0)
        {
            sle = T::map()[errorCode];
            return true;
        }
        return false;
    }
    ///
    /// Retrieves the login status code based on the error string
    /// linear time lookup
    ///
    template <typename T>
    static Core::LoginStatus loginErrorCodeFromStr(const QString& errorString)
    {
        T::createMap();
        Core::LoginStatus ls = Core::LoginStatusErrorUnknown;
        foreach(T sle, T::map())
        {
            if (sle.eString() == errorString)
            {
                ls = sle.loginStatus();
                break;
            }
        }
        return ls;
    }

protected:
    QString mErrorCode;
    QString mErrorString;
    QString mMessageString;
    Core::LoginStatus mLoginStatus;
protected:
    ///
    /// set helper
    ///
    void set(const LoginError& sle)
    {
        mErrorCode = sle.eCode();
        mErrorString = sle.eString();
        mLoginStatus = sle.loginStatus();
        mMessageString = sle.mString();
    }

};


class ORIGIN_PLUGIN_API ClientLoginError: public LoginError
{
    Q_OBJECT

public:

    ///
    /// CTOR
    ///
    ClientLoginError(const QString& errorCode, const QString& errorString, Core::LoginStatus loginStatus, const QString& messageString = QString() )		
        :LoginError(errorCode, errorString,loginStatus,messageString)
    {
    }
    ///
    /// Default CTOR
    ///
    ClientLoginError()
    {

    }

    virtual ClientLoginError& operator=(const LoginError& sle)
    {
        if (this != &sle)
        {
            set(sle);
        }
        return *this;
    }

    ///
    /// This function will create the map the first time it is called.
    ///
    static void createMap()
    {
        // Map creation
        if (mMap.size() != 0)
            return;

        using namespace Core;

        mMap[C_USER_LOGIN_ERROR_UNKNOWN] = ClientLoginError(C_USER_LOGIN_ERROR_UNKNOWN, C_USER_LOGIN_ERROR_UNKNOWN, LoginStatusErrorUnknown);
        mMap[C_USER_LOGIN_SUCCESS] = ClientLoginError(C_USER_LOGIN_SUCCESS,C_USER_LOGIN_SUCCESS, LoginStatusSuccess);
        mMap[C_USER_LOGIN_SUCCESS_ONLINE] = ClientLoginError(C_USER_LOGIN_SUCCESS_ONLINE,C_USER_LOGIN_SUCCESS_ONLINE, LoginStatusSuccessOnline);
        mMap[C_USER_LOGIN_SUCCESS_OFFLINE] = ClientLoginError(C_USER_LOGIN_SUCCESS_OFFLINE,C_USER_LOGIN_SUCCESS_OFFLINE, LoginStatusSuccessOffline);
        mMap[C_USER_LOGIN_CLIENT_ERROR_NO_CONNECTION] = ClientLoginError(C_USER_LOGIN_CLIENT_ERROR_NO_CONNECTION,C_USER_LOGIN_CLIENT_ERROR_NO_CONNECTION, LoginStatusErrorNoConnection);
        mMap[C_USER_LOGIN_CLIENT_ERROR_NEVER_CONNECTED] = ClientLoginError(C_USER_LOGIN_CLIENT_ERROR_NEVER_CONNECTED,C_USER_LOGIN_CLIENT_ERROR_NEVER_CONNECTED, LoginStatusErrorUserNeverConnected);
        mMap[C_USER_LOGIN_CLIENT_ERROR_DIP_DIR_INVALID] = ClientLoginError(C_USER_LOGIN_CLIENT_ERROR_DIP_DIR_INVALID,C_USER_LOGIN_CLIENT_ERROR_DIP_DIR_INVALID, LoginStatusErrorDipDirInvalid);
        mMap[C_USER_LOGIN_CLIENT_ERROR_CACHE_DIR_INVALID] = ClientLoginError(C_USER_LOGIN_CLIENT_ERROR_CACHE_DIR_INVALID,C_USER_LOGIN_CLIENT_ERROR_CACHE_DIR_INVALID, LoginStatusErrorCacheDirInvalid);
        mMap[C_USER_LOGIN_CLIENT_ERROR_PASSWORD_TOKEN] = ClientLoginError(C_USER_LOGIN_CLIENT_ERROR_PASSWORD_TOKEN,C_USER_LOGIN_CLIENT_ERROR_PASSWORD_TOKEN, LoginStatusErrorPWorAuthtoken);
       
    }
    static QHash< QString, ClientLoginError >& map() {createMap(); return mMap;}

private:
    ///
    /// Map of error codes, strings and tr messages
    ///
    static QHash< QString, ClientLoginError > mMap;
};

class ORIGIN_PLUGIN_API ServerStatusError: public LoginError
{
    Q_OBJECT

public:

    ///
    /// CTOR
    ///
    ServerStatusError(const QString& errorCode, const QString& errorString, Core::LoginStatus loginStatus, const QString& messageString = QString() )		
        :LoginError(errorCode, errorString,loginStatus,messageString)
    {
    }
    ///
    /// Default CTOR
    ///
    ServerStatusError()
    {

    }

    virtual ServerStatusError& operator=(const LoginError& sle)
    {
        if (this != &sle)
        {
            set(sle);
        }
        return *this;
    }

    ///
    /// This function will create the map the first time it is called.
    ///
    static void createMap()
    {
        // Map creation
        if (mMap.size() != 0)
            return;

        using namespace Core;

        mMap[C_USER_SERVER_STATUS_CODE_VOLATILE]		= ServerStatusError(C_USER_SERVER_STATUS_CODE_VOLATILE, C_USER_SERVER_STATUS_CODE_VOLATILE_TELEMETRY, LoginStatusErrorUserVolatile);
        mMap[C_USER_SERVER_STATUS_CODE_TENTATIVE]		= ServerStatusError(C_USER_SERVER_STATUS_CODE_TENTATIVE,C_USER_SERVER_STATUS_CODE_TENTATIVE_TELEMETRY, LoginStatusErrorUserTentative);
        mMap[C_USER_SERVER_STATUS_CODE_CHILD_PENDING]	= ServerStatusError(C_USER_SERVER_STATUS_CODE_CHILD_PENDING,C_USER_SERVER_STATUS_CODE_CHILD_PENDING_TELEMETRY, LoginStatusErrorAccountPending);
        mMap[C_USER_SERVER_STATUS_CODE_DELETED]			= ServerStatusError(C_USER_SERVER_STATUS_CODE_DELETED,C_USER_SERVER_STATUS_CODE_DELETED_TELEMETRY, LoginStatusErrorAccountDeleted);
        mMap[C_USER_SERVER_STATUS_CODE_DEACTIVATED]		= ServerStatusError(C_USER_SERVER_STATUS_CODE_DEACTIVATED,C_USER_SERVER_STATUS_CODE_DEACTIVATED_TELEMETRY, LoginStatusErrorAccountDeactivated);
        mMap[C_USER_SERVER_STATUS_CODE_BANNED]			= ServerStatusError(C_USER_SERVER_STATUS_CODE_BANNED,C_USER_SERVER_STATUS_CODE_BANNED_TELEMETRY, LoginStatusErrorAccountBanned);
        mMap[C_USER_SERVER_STATUS_CODE_DISABLED]		= ServerStatusError(C_USER_SERVER_STATUS_CODE_DISABLED,C_USER_SERVER_STATUS_CODE_DISABLED_TELEMETRY, LoginStatusErrorAccountDisabled);
    }
    static QHash< QString, ServerStatusError >& map() { createMap(); return mMap;}

private:
    ///
    /// Map of error codes, strings and tr messages
    ///
    static QHash< QString, ServerStatusError > mMap;
};

class ORIGIN_PLUGIN_API ServerLoginError: public LoginError
{
    Q_OBJECT

public:

    ///
    /// CTOR
    ///
    ServerLoginError(const QString& errorCode, const QString& errorString, Core::LoginStatus loginStatus, const QString& messageString = QString() )		
        :LoginError(errorCode, errorString,loginStatus,messageString)
    {
    }
    ///
    /// Default CTOR
    ///
    ServerLoginError()
    {

    }

    virtual ServerLoginError& operator=(const LoginError& sle)
    {
        if (this != &sle)
        {
            set(sle);
        }
        return *this;
    }

    ///
    /// This function will create the map the first time it is called.
    ///
    static void createMap()
    {
        // Map creation
        if (mMap.size() != 0)
            return;

        using namespace Core;
        /****************** Service Internal Exception Error Code *******************/
        // Error code when rs4 exception happens
        mMap[C_USER_LOGIN_ERROR_CODE_RS4_EXCEPTION] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_RS4_EXCEPTION, C_USER_LOGIN_ERROR_TEXT_RS4_EXCEPTION, LoginStatusErrorRS4Exception);

        // Error code when connect nucleus time out.
        mMap[C_USER_LOGIN_ERROR_CODE_NUCLEUS_TIMEOUT] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NUCLEUS_TIMEOUT,C_USER_LOGIN_ERROR_TEXT_NUCLEUS_TIMEOUT,LoginStatusErrorNucleusTimeout);

        // Error code when access send email service.
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL,C_USER_LOGIN_ERROR_TEXT_EMAIL,LoginStatusErrorEmail);

        /********************** Business Exception Error Code ***********************/
        //email from client is empty
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL_EMPTY,C_USER_LOGIN_ERROR_TEXT_EMAIL_EMPTY,LoginStatusErrorEmailEmpty);

        // Error code when password from client is empty
        mMap[C_USER_LOGIN_ERROR_CODE_PASSWORD_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PASSWORD_EMPTY,C_USER_LOGIN_ERROR_TEXT_PASSWORD_EMPTY,LoginStatusErrorPasswordEmpty);

        // Error code when there's no user found in server
        mMap[C_USER_LOGIN_ERROR_CODE_NO_USER] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NO_USER,C_USER_LOGIN_ERROR_TEXT_NO_USER,LoginStatusErrorNoUser);

        // Error code when email and password from client are not matched
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL_PASSWORD_NOT_MATCH] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL_PASSWORD_NOT_MATCH,C_USER_LOGIN_ERROR_TEXT_EMAIL_PASSWORD_NOT_MATCH,LoginStatusErrorEmailPasswordNoMatch);

        // Error code when registration for user from Germany has error
        mMap[C_USER_LOGIN_ERROR_CODE_GERMANY_CHECK] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_GERMANY_CHECK,C_USER_LOGIN_ERROR_TEXT_GERMANY_CHECK,LoginStatusErrorGermanyCheck);

        // Error code when email format from client is incorrect
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_EMAIL_ILLEGAL,LoginStatusErrorEmaiIllegal);

        // Error code when email has exist in server
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL_EXIST] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL_EXIST,C_USER_LOGIN_ERROR_TEXT_EMAIL_EXISTS,LoginStatusErrorEmailExists);

        // Error code when length of password from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_PASSWORD_LENGTH] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PASSWORD_LENGTH,C_USER_LOGIN_ERROR_TEXT_PASSWORD_LENGTH,LoginStatusErrorInvalidPasswordLength);

        // Error code when password format from client is incorrect
        mMap[C_USER_LOGIN_ERROR_CODE_PASSWORD_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PASSWORD_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_PASSWORD_ILLEGAL,LoginStatusErrorWrongPassword);

        // Error code when country from client is empty
        mMap[C_USER_LOGIN_ERROR_CODE_COUNTRY_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_COUNTRY_EMPTY,C_USER_LOGIN_ERROR_TEXT_COUNTRY_EMPTY,LoginStatusErrorCountryCodeNotSupplied);

        // Error code when country client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_COUNTRY_CODE ] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_COUNTRY_CODE ,C_USER_LOGIN_ERROR_TEXT_COUNTRY,LoginStatusErrorCountryDoesNotExist);

        // Error code when dob from client is empty
        mMap[C_USER_LOGIN_ERROR_CODE_DOB_EMPTY ] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_DOB_EMPTY ,C_USER_LOGIN_ERROR_TEXT_DOB_EMPTY,LoginStatusErrorDOBNotSupplied);

        // Error code when dob from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_DOB_ILLEGAL ] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_DOB_ILLEGAL ,C_USER_LOGIN_ERROR_TEXT_DOB_ILLEGAL,LoginStatusErrorDOBIllegal);

        // Error code when Law Age from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_AGE_LAW ] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_AGE_LAW ,C_USER_LOGIN_ERROR_TEXT_AGE_LAW,LoginStatusErrorUserIsUnderAge);

        // Error code when thirdpartyOptin from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_THIRDPARTYOPTIN_ILLEGAL ] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_THIRDPARTYOPTIN_ILLEGAL ,C_USER_LOGIN_ERROR_TEXT_THIRDPARTYOPTIN_ILLEGAL,LoginStatusErrorThirdPartyOptInIllegal);

        // Error code when globalOptin from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_GLOBALOPTIN_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_GLOBALOPTIN_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_GLOBALOPTIN_ILLEGAL,LoginStatusErrorGlobalPartyOptInIllegal);

        // Error code when tosVersion from client is empty
        mMap[C_USER_LOGIN_ERROR_CODE_TOS_VERSION_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOS_VERSION_EMPTY,C_USER_LOGIN_ERROR_TEXT_TOS_VERSION_EMPTY,LoginStatusErrorTOSVersionEmpty);

        // Error code when value of status from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_STATUS_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_STATUS_EMPTY,C_USER_LOGIN_ERROR_TEXT_STATUS_EMPTY,LoginStatusErrorStatusEmpty);

        // Error code when EAID from client is empty
        mMap[C_USER_LOGIN_ERROR_CODE_EAID_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EAID_EMPTY,C_USER_LOGIN_ERROR_TEXT_EAID_EMPTY,LoginStatusErrorEAIDParameterEmpty, tr("ebisu_client_eaid_error_empty").arg(tr("ebisu_client_master_id")));

        // Error code when EAID from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_EAID_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EAID_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_EAID_ILLEGAL,LoginStatusErrorEAIDParameterInvalid, tr("ebisu_client_eaid_error_illegal"));

        // Error code when EAID has exist
        mMap[C_USER_LOGIN_ERROR_CODE_PERSONA_NAME_EXIST] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PERSONA_NAME_EXIST,C_USER_LOGIN_ERROR_TEXT_PERSONA_NAME_EXIST,LoginStatusErrorPersonaNameExists, tr("ebisu_client_eaid_error_already_exist").arg(tr("ebisu_client_master_id")));

        // Error code when EAID contains dirty word
        mMap[C_USER_LOGIN_ERROR_CODE_CONTAINS_DIRTY_WORD] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CONTAINS_DIRTY_WORD,C_USER_LOGIN_ERROR_TEXT_CONTAINS_DIRTY_WORD,LoginStatusErrorCodeContainsDirtyWord);

        // Error code when attempt to save user and error happens
        mMap[C_USER_LOGIN_ERROR_CODE_SAVE_USER] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_SAVE_USER,C_USER_LOGIN_ERROR_TEXT_SAVE_USER,LoginStatusErrorSaveUser);

        // Error code when create EAID failed
        mMap[C_USER_LOGIN_ERROR_CODE_SAVE_PERSONA] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_SAVE_PERSONA,C_USER_LOGIN_ERROR_TEXT_SAVE_PERSONA,LoginStatusErrorSavePersona);

        // Error code when send the Email
        mMap[C_USER_LOGIN_ERROR_CODE_SEND_EMAIL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_SEND_EMAIL,C_USER_LOGIN_ERROR_TEXT_SEND_EMAIL,LoginStatusErrorSendEmail);

        // Error code when active Germany double option filed
        mMap[C_USER_LOGIN_ERROR_CODE_ACTIVE_GERMANY_FAILURE] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_ACTIVE_GERMANY_FAILURE,C_USER_LOGIN_ERROR_TEXT_ACTIVE_GERMANY_FAILURE,LoginStatusErrorGermanyFailure);

        // Error code when value of status from client is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_STATUS_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_STATUS_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_STATUS_ILLEGAL,LoginStatusErrorStatusIllegal);

        // Error code when email from client is not exist in server
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL_NOT_EXIST] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL_NOT_EXIST,C_USER_LOGIN_ERROR_TEXT_EMAIL_NOT_EXIST,LoginStatusErrorNoUserExistForThatEmail);

        // ******************* validate key *********************//

        // Error code when failed to generate validate key for user
        mMap[C_USER_LOGIN_ERROR_CODE_GENERATE_KEY_FAIL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_GENERATE_KEY_FAIL,C_USER_LOGIN_ERROR_TEXT_GENERATE_KEY_FAIL,LoginStatusErrorGenerateKeyFail);

        // Error code when EAID from client is not exist in server
        mMap[C_USER_LOGIN_ERROR_CODE_EAID_NOT_EXIST] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EAID_NOT_EXIST,C_USER_LOGIN_ERROR_TEXT_EAID_NOT_EXIST,LoginStatusErrorEAIDNotExist);

        // Error code when no validate key exist in database
        mMap[C_USER_LOGIN_ERROR_CODE_NO_VALIDATE_KEY_IN_DB] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NO_VALIDATE_KEY_IN_DB,C_USER_LOGIN_ERROR_TEXT_NO_VALIDATE_KEY_IN_DB,LoginStatusErrorNoValidateKeyInDB);

        // Error code when double check error
        mMap[C_USER_LOGIN_ERROR_CODE_DOUBLE_CHECK_ERROR] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_DOUBLE_CHECK_ERROR,C_USER_LOGIN_ERROR_TEXT_DOUBLE_CHECK_ERROR,LoginStatusErrorDoubleCheckError);

        // Error code when nucleus id and personal id are all empty
        mMap[C_USER_LOGIN_ERROR_CODE_NUCLEUSID_AND_PERSONAID_ALL_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NUCLEUSID_AND_PERSONAID_ALL_EMPTY,C_USER_LOGIN_ERROR_TEXT_NUCLEUSID_AND_PERSONAID_ALL_EMPTY,LoginStatusErrorNucleusAndPersonaIdAllEmpty);

        mMap[C_USER_LOGIN_ERROR_CODE_REQUEST_AUTHTOKEN_INVALID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_REQUEST_AUTHTOKEN_INVALID,C_USER_LOGIN_ERROR_TEXT_REQUEST_AUTHTOKEN_INVALID,LoginStatusErrorTokenInvalidOrEmpty, tr("ebisu_client_eaid_error_invalid_token"));

        // Error C_USER_LOGIN_ERROR_CODE_REQUEST_AUTHTOKEN_INVALIDAID from client isn't 4-16
        mMap[C_USER_LOGIN_ERROR_CODE_EAID_LENGTH] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EAID_LENGTH,C_USER_LOGIN_ERROR_TEXT_EAID_LENGTH,LoginStatusErrorInvalidEAIDLength, tr("ebisu_client_eaid_error_length").arg(tr("ebisu_client_master_id")));

        // Error code when user's language is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_LANGUAGE_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_LANGUAGE_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_LANGUAGE_ILLEGAL,LoginStatusErrorLanguageIllegal);

        // Error code when user's tosAccepted is illegal (length should be 1-255)
        mMap[C_USER_LOGIN_ERROR_CODE_TOSACCEPTED_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOSACCEPTED_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_TOSACCEPTED_ILLEGAL,LoginStatusErrorTOSAcceptedIllegal);

        // Error code when user's tosVersion is illegal (length should be 1-255)
        mMap[C_USER_LOGIN_ERROR_CODE_TOSVERSION_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOSVERSION_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_TOSVERSION_ILLEGAL,LoginStatusErrorTOSVersionIllegal);

        // Error code when user's information contains invalid character (e.g. &, < , > )
        mMap[C_USER_LOGIN_ERROR_CODE_SPECIAL_CHARACTER] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_SPECIAL_CHARACTER,C_USER_LOGIN_ERROR_TEXT_SPECIAL_CHARACTER,LoginStatusErrorSpecialCharacter);

        // Error code when user's locale is illegal
        mMap[C_USER_LOGIN_ERROR_CODE_LOCALE_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_LOCALE_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_LOCALE_ILLEGAL,LoginStatusErrorInvalidLocale);

        // Error code when translate url character error
        mMap[C_USER_LOGIN_ERROR_CODE_TRANSLATE_URL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TRANSLATE_URL,C_USER_LOGIN_ERROR_TEXT_TRANSLATE_URL,LoginStatusErrorTranslatedURL);

        // Error code when create too many personas in same namespace for user.
        mMap[C_USER_LOGIN_ERROR_CODE_TOO_MANY_PERSONAS] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOO_MANY_PERSONAS,C_USER_LOGIN_ERROR_TEXT_TOO_MANY_PERSONAS,LoginStatusErrorTooManyPersonasSameNS, tr("ebisu_client_eaid_error_too_many_personas"));

        // Error code when nucleus conflicts
        mMap[C_USER_LOGIN_ERROR_CODE_GENERIC_NUCLEUS_ERROR] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_GENERIC_NUCLEUS_ERROR,C_USER_LOGIN_ERROR_TEXT_GENERIC_NUCLEUS_ERROR,LoginStatusErrorGenericNucleusError);

        // Error code when first name too long(max length 255 characters)
        mMap[C_USER_LOGIN_ERROR_CODE_FIRST_NAME_TOO_LONG] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_FIRST_NAME_TOO_LONG,C_USER_LOGIN_ERROR_TEXT_FIRST_NAME_TOO_LONG,LoginStatusErrorFirstNameIsTooLong);

        // Error code when last name too long(max length 255 characters)
        mMap[C_USER_LOGIN_ERROR_CODE_LAST_NAME_TOO_LONG] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_LAST_NAME_TOO_LONG,C_USER_LOGIN_ERROR_TEXT_LAST_NAME_TOO_LONG,LoginStatusErrorLastNameIsTooLong);

        // Error code when user has created first name and last name.
        mMap[C_USER_LOGIN_ERROR_CODE_NAME_DUPLICATE_VALUE] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NAME_DUPLICATE_VALUE,C_USER_LOGIN_ERROR_TEXT_NAME_DUPLICATE_VALUE,LoginStatusErrorNameDuplicateValue);

        // Error code when user update not exist profile.
        mMap[C_USER_LOGIN_ERROR_CODE_NO_SUCH_PROFILE_INFO] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NO_SUCH_PROFILE_INFO,C_USER_LOGIN_ERROR_TEXT_NO_SUCH_PROFILE_INFO,LoginStatusErrorNoSuchProfileInfo);

        // Error code when first name or last name is null or empty
        mMap[C_USER_LOGIN_ERROR_CODE_FIRST_LAST_NAME_NULL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_FIRST_LAST_NAME_NULL,C_USER_LOGIN_ERROR_TEXT_FIRST_LAST_NAME_NULL,LoginStatusErrorFirstLastNameNull);

        // Error code when privacy setting key is not supported
        mMap[C_USER_LOGIN_ERROR_CODE_SETTING_KEY_NOT_SUPPORTED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_SETTING_KEY_NOT_SUPPORTED,C_USER_LOGIN_ERROR_TEXT_SETTING_KEY_NOT_SUPPORTED,LoginStatusErrorPrivacySettingNotSupported);

        // Error code when user's privacy setting is not found in DB
        mMap[C_USER_LOGIN_ERROR_CODE_PRIVACY_SETTING_NOT_FOUND] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PRIVACY_SETTING_NOT_FOUND,C_USER_LOGIN_ERROR_TEXT_PRIVACY_SETTING_NOT_FOUND,LoginStatusErrorPrivacySettingNotFound);

        // Error code when token expired
        mMap[C_USER_LOGIN_ERROR_CODE_TOKEN_EXPIRED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOKEN_EXPIRED,C_USER_LOGIN_ERROR_TEXT_TOKEN_EXPIRED,LoginStatusErrorTokenExpired);

        // Error code when token invalid
        mMap[C_USER_LOGIN_ERROR_CODE_TOKEN_INVALID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOKEN_INVALID,C_USER_LOGIN_ERROR_TEXT_TOKEN_INVALID,LoginStatusErrorTokenInvalid);

        // Error code when parameter of nucleusId is not number
        mMap[C_USER_LOGIN_ERROR_CODE_NUCLEUS_ID_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NUCLEUS_ID_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_NUCLEUS_ID_ILLEGAL,LoginStatusErrorIllegalNucleusId);

        // Error code when parameter of personaId is not number
        mMap[C_USER_LOGIN_ERROR_CODE_PERSONA_ID_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PERSONA_ID_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_PERSONA_ID_ILLEGAL,LoginStatusErrorPersonaIdIllegal);

        // Error code when parameter of userId is not number
        mMap[C_USER_LOGIN_ERROR_CODE_USER_ID_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_USER_ID_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_USER_ID_ILLEGAL,LoginStatusErrorIllegalUserId);

        // Error code when userId and AuthToken are not consistent
        mMap[C_USER_LOGIN_ERROR_CODE_TOKEN_USERID_DIFFER] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_TOKEN_USERID_DIFFER,C_USER_LOGIN_ERROR_TEXT_TOKEN_USERID_DIFFER,LoginStatusErrorUserIdMismatchAuthToken);

        // Error code when user profile is not found in DB
        mMap[C_USER_LOGIN_ERROR_CODE_USERPROFILE_NOT_FOUND] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_USERPROFILE_NOT_FOUND,C_USER_LOGIN_ERROR_TEXT_USERPROFILE_NOT_FOUND,LoginStatusErrorUserProfileNotFound);

        // Error code when the user this reference id has been associated with another nucleus user already.
        mMap[C_USER_LOGIN_ERROR_CODE_EXTERNAL_REFERENCE_ID_DUPLICATE] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EXTERNAL_REFERENCE_ID_DUPLICATE,C_USER_LOGIN_ERROR_TEXT_EXTERNAL_REFERENCE_ID_DUPLICATE,LoginStatusErrorExternalReferenceIdDuplicate);

        // Error code when option name is not supported
        mMap[C_USER_LOGIN_ERROR_CODE_OPTION_NOT_SUPPORTED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_OPTION_NOT_SUPPORTED,C_USER_LOGIN_ERROR_TEXT_OPTION_NOT_SUPPORTED,LoginStatusErrorCodeOptionNotSupported);

        // Error code when parameter userIds is empty.
        mMap[C_USER_LOGIN_ERROR_CODE_USER_IDS_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_USER_IDS_EMPTY,C_USER_LOGIN_ERROR_TEXT_USER_IDS_EMPTY,LoginStatusErrorUserIDsInvalidOrEmpty);

        // Error code when parameter userId is empty or invalid
        mMap[C_USER_LOGIN_ERROR_CODE_PARAM_USERID_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PARAM_USERID_EMPTY,C_USER_LOGIN_ERROR_TEXT_PARAM_USERID_EMPTY,LoginStatusErrorUserIDEmpty);

        // Error code when action name is not supported
        mMap[C_USER_LOGIN_ERROR_CODE_ACTION_NOT_SUPPORTED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_ACTION_NOT_SUPPORTED,C_USER_LOGIN_ERROR_TEXT_ACTION_NOT_SUPPORTED,LoginStatusErrorActionNotSupported);

        // Error code when extra info is too long
        mMap[C_USER_LOGIN_ERROR_CODE_EXTRA_INFO_TOO_LONG] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EXTRA_INFO_TOO_LONG,C_USER_LOGIN_ERROR_TEXT_EXTRA_INFO_TOO_LONG,LoginStatusErrorInfoToolONG);

        // Error code when customer name is empty
        mMap[C_USER_LOGIN_ERROR_CODE_CUSTOMER_NAME_EMPTY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CUSTOMER_NAME_EMPTY,C_USER_LOGIN_ERROR_TEXT_CUSTOMER_NAME_EMPTY,LoginStatusErrorCustomerNameEmpty);

        // Error code when friend's id is not number
        mMap[C_USER_LOGIN_ERROR_CODE_FRIEND_ID_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_FRIEND_ID_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_FRIEND_ID_ILLEGAL,LoginStatusErrorFriendIdIllegal);

        // Error code when customer name is too long
        mMap[C_USER_LOGIN_ERROR_CODE_CUSTOMER_NAME_TOO_LONG] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CUSTOMER_NAME_TOO_LONG,C_USER_LOGIN_ERROR_TEXT_CUSTOMER_NAME_TOO_LONG,LoginStatusErrorCustomerNameTooLong);

        // Error code parameter invalid.
        mMap[C_USER_LOGIN_ERROR_CODE_PARAM_INVALID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PARAM_INVALID,C_USER_LOGIN_ERROR_TEXT_PARAM_INVALID,LoginStatusErrorCodeParamInvalid);

        // Error code when delete or update a parent row: a foreign key constraint fails (%s)
        mMap[C_USER_LOGIN_ERROR_CODE_PARENT_FOREIGN_KEY_CONSTRAINT] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_PARENT_FOREIGN_KEY_CONSTRAINT,C_USER_LOGIN_ERROR_TEXT_PARENT_FOREIGN_KEY_CONSTRAINT,LoginStatusErrorParentForeignKeyConstraint);

        // Error code when add or update a child row: a foreign key constraint fails (%s)
        mMap[C_USER_LOGIN_ERROR_CODE_CHILD_FOREIGN_KEY_CONSTRAINT] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CHILD_FOREIGN_KEY_CONSTRAINT,C_USER_LOGIN_ERROR_TEXT_CHILD_FOREIGN_KEY_CONSTRAINT,LoginStatusErrorChildForeignKeyConstraint);

        // Error code when Duplicate entry '%s' for key %d
        mMap[C_USER_LOGIN_ERROR_CODE_DUPLICATE_ENTRY] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_DUPLICATE_ENTRY,C_USER_LOGIN_ERROR_TEXT_DUPLICATE_ENTRY,LoginStatusErrorDuplicateEntry);

        // Error code when the specify referenceType is not exist.
        mMap[C_USER_LOGIN_ERROR_CODE_NO_SUCH_REFERENCE_TYPE] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NO_SUCH_REFERENCE_TYPE,C_USER_LOGIN_ERROR_TEXT_NO_SUCH_REFERENCE_TYPE,LoginStatusErrorReferenceTypeInvalid);

        // Error code when update password but email not consistent with userEmail.
        mMap[C_USER_LOGIN_ERROR_CODE_EMAIL_NOT_CONSISTENT] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_EMAIL_NOT_CONSISTENT,C_USER_LOGIN_ERROR_TEXT_EMAIL_NOT_CONSISTENT,LoginStatusErrorEmailNotConsistent);

        // Error code when user's RegistrationSource is illegal (length should be 1-64)
        mMap[C_USER_LOGIN_ERROR_CODE_REGISTRATIONSOURCE_ILLEGAL] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_REGISTRATIONSOURCE_ILLEGAL,C_USER_LOGIN_ERROR_TEXT_REGISTRATIONSOURCE_ILLEGAL,LoginStatusErrorRegistrationSourceIllegal);

        // Error code when user's dob time is in future.
        mMap[C_USER_LOGIN_ERROR_CODE_DATE_OF_BIRTH_INVALID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_DATE_OF_BIRTH_INVALID,C_USER_LOGIN_ERROR_TEXT_DATE_OF_BIRTH_INVALID,LoginStatusErrorDOBInvalid);

        // Error code when an unrecognized registrationsource is not provided by Nucleus.
        mMap[C_USER_LOGIN_ERROR_CODE_REGISTRATIONSOURCE_UNRECOGNIZED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_REGISTRATIONSOURCE_UNRECOGNIZED,C_USER_LOGIN_ERROR_TEXT_REGISTRATIONSOURCE_UNRECOGNIZED,LoginStatusErrorRegistrationUnrecognized);

        // Error code parameter email or ea_login_id is empty
        mMap[C_USER_LOGIN_ERROR_CODE_MISSING_EMAIL_OR_EALOGINID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_MISSING_EMAIL_OR_EALOGINID,C_USER_LOGIN_ERROR_TEXT_MISSING_EMAIL_OR_EALOGINID,LoginStatusErrorEmailOrEAIDMissing);

        // Error code when the user trying to do forgot password with a weak password when the setting on nuclues is strong
        mMap[C_USER_LOGIN_ERROR_CODE_WEAK_PASSWORD] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_WEAK_PASSWORD,C_USER_LOGIN_ERROR_TEXT_WEAK_PASSWORD,LoginStatusErrorWeakPassword);

        // Error code parameter email or ea_login_id is empty
        mMap[C_USER_LOGIN_ERROR_CODE_MISSING_EALOGINID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_MISSING_EALOGINID,C_USER_LOGIN_ERROR_TEXT_MISSING_EALOGINID,LoginStatusErrorEmailOrEAIDMissing);

        // Error code when access blaze server error
        mMap[C_USER_LOGIN_ERROR_CODE_ACCESS_BLAZE] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_ACCESS_BLAZE,C_USER_LOGIN_ERROR_TEXT_ACCESS_BLAZE,LoginStatusErrorAccessBlaze);

        // Error code when credential not exist in cache
        mMap[C_USER_LOGIN_ERROR_CODE_CREDENTIAL_NOT_EXIST] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CREDENTIAL_NOT_EXIST,C_USER_LOGIN_ERROR_TEXT_CREDENTIAL_NOT_EXIST,LoginStatusErrorCredentialNotExist);

        // Error code when the parameter of blaze not supplied
        mMap[C_USER_LOGIN_ERROR_CODE_BLAZE_ID_INVALID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_BLAZE_ID_INVALID,C_USER_LOGIN_ERROR_TEXT_BLAZE_ID_INVALID,LoginStatusErrorBlazeIdInvalid);

        // Error code when the parameter of friendIds not supplied.
        mMap[C_USER_LOGIN_ERROR_CODE_FRIENDS_ID_MISSING] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_FRIENDS_ID_MISSING,C_USER_LOGIN_ERROR_TEXT_FRIENDS_ID_MISSING,LoginStatusErrorFriendsIdMissing);

        // Error code when the auth server is requiring a captcha for this account.
        mMap[C_USER_LOGIN_ERROR_CODE_CAPTCHA_REQUIRED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CAPTCHA_REQUIRED, C_USER_LOGIN_ERROR_TEXT_CAPTCHA_REQUIRED,LoginStatusErrorCaptchaRequired);

        // Error code when captcha entered is incorrect
        mMap[C_USER_LOGIN_ERROR_CODE_CAPTCHA_INCORRECT] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_CAPTCHA_INCORRECT, C_USER_LOGIN_ERROR_TEXT_CAPTCHA_INCORRECT,LoginStatusErrorCaptchaIncorrect);

        // Error code bonehead password is encountered
        mMap[C_USER_LOGIN_ERROR_MUST_CHANGE_PASSWORD] = ServerLoginError(C_USER_LOGIN_ERROR_MUST_CHANGE_PASSWORD, C_USER_LOGIN_ERROR_TEXT_MUST_CHANGE_PASSWORD,LoginStatusErrorMustChangePassword);

        // added Match 2012
        mMap[C_USER_LOGIN_ERROR_CODE_MISSING_X_ORIGIN_UID] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_MISSING_X_ORIGIN_UID, C_USER_LOGIN_ERROR_TEXT_MISSING_X_ORIGIN_UID,LoginStatusErrorMissingXOriginUID);
        mMap[C_USER_LOGIN_ERROR_CODE_X_ORIGIN_UID_CHECKSUM_MISMATCH] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_X_ORIGIN_UID_CHECKSUM_MISMATCH, C_USER_LOGIN_ERROR_TEXT_X_ORIGIN_UID_CHECKSUM_MISMATCH,LoginStatusErrorXOriginUIDCheckSumMistmatch);
        mMap[C_USER_LOGIN_ERROR_CODE_INVALID_MACHINE_HASH] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_INVALID_MACHINE_HASH, C_USER_LOGIN_ERROR_TEXT_INVALID_MACHINE_HASH,LoginStatusErrorInvalidMachineHash);
        mMap[C_USER_LOGIN_ERROR_CODE_NUCLEUS_SECURITY_STATE_RED] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_NUCLEUS_SECURITY_STATE_RED, C_USER_LOGIN_ERROR_TEXT_NUCLEUS_SECURITY_STATE_RED,LoginStatusErrorNucleusSecurityStateRed);
        mMap[C_USER_LOGIN_ERROR_CODE_FORBIDDEN] = ServerLoginError(C_USER_LOGIN_ERROR_CODE_FORBIDDEN, C_USER_LOGIN_ERROR_TEXT_FORBIDDEN,LoginStatusErrorCodeForbidden);
    }
    static QHash< QString, ServerLoginError >& map() {createMap(); return mMap;}

private:
    ///
    /// Map of error codes, strings and tr messages
    ///
    static QHash< QString, ServerLoginError > mMap;

};

ORIGIN_PLUGIN_API static Core::LoginStatus loginStatus(const QString& errorStr)
{
    using namespace Core;
    LoginStatus  eResponse_ret = LoginStatusErrorUnknown;
    eResponse_ret = LoginError::loginErrorCodeFromStr<ClientLoginError>(errorStr);
    if (LoginStatusErrorUnknown == eResponse_ret)
    {
        eResponse_ret = LoginError::loginErrorCodeFromStr<ServerLoginError>(errorStr);

    }
    if (LoginStatusErrorUnknown == eResponse_ret)
    {
        eResponse_ret = LoginError::loginErrorCodeFromStr<ServerStatusError>(errorStr);

    }
    return eResponse_ret;
}

ORIGIN_PLUGIN_API static QString errorString(Core::LoginStatus eaCoreStatus)
{
    QString reason = LoginError::errorString<ServerLoginError>(eaCoreStatus);
    if (reason.isEmpty())
    {
        // ...else look in the client
        reason = LoginError::errorString<ClientLoginError>(eaCoreStatus);
    }
    // lastly, check the server status error
    if (reason.isEmpty())
    {
        // ...else look in the client
        reason = LoginError::errorString<ServerStatusError>(eaCoreStatus);
    }
    return reason;
}

ORIGIN_PLUGIN_API static QString errorString(const QString& svrError, Core::LoginStatus eaCoreStatus)
{
    QString reason = LoginError::errorString<ServerLoginError>(svrError);
    if (reason.isEmpty())
    {
        reason = errorString(eaCoreStatus);
    }
    return reason;
}


#endif
