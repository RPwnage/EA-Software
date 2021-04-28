///////////////////////////////////////////////////////////////////////////////
// OriginServiceValues.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _ORIGINSERVICEVALUES_H_INCLUDED_
#define _ORIGINSERVICEVALUES_H_INCLUDED_

//	RESTLOGINERROR(rest enum, enum values, error string)
#define RESTLOGINERRORS\
    RESTLOGINERROR(restErrorInvalidBuildIdOverride,                     -15004, "INVALID_BUILD_ID_OVERRIDE")\
    RESTLOGINERROR(restErrorSessionExpired,                             -10,"SESSION_EXPIRED")\
    RESTLOGINERROR(restErrorNetwork_TemporaryNetworkFailure,            -9,"NETWORK_TEMPORARYNETWORKFAILURE")\
    RESTLOGINERROR(restErrorNetwork_SslHandshakeFailed,                 -8,"NETWORK_SSLHANDSHAKEFAILED")\
    RESTLOGINERROR(restErrorNetWork_OperationCanceledError,             -7,"NETWORK_OPERATIONCANCELED")\
    RESTLOGINERROR(restErrorNetwork_TimeoutError,                       -6,"NETWORK_TIMEOUT")\
    RESTLOGINERROR(restErrorNetwork_HostNotFound,                       -5,"NETWORK_HOSTNOTFOUND")\
    RESTLOGINERROR(restErrorNetwork_RemoteHostClosed,                   -4,"NETWORK_REMOTEHOSTCLOSED")\
    RESTLOGINERROR(restErrorNetwork_ConnectionRefused,                  -3,"NETWORK_CONNECTIONREFUSED")\
    RESTLOGINERROR(restErrorNetwork,                                    -2,"NETWORK")\
    RESTLOGINERROR(restErrorUnknown,                                    -1,"UNKNOWN")\
    RESTLOGINERROR(restErrorSuccess,                                         0,"SUCCESS")\
    RESTLOGINERROR(restErrorInternalServerError,                           504,"INTERNALSERVERERROR")\
    RESTLOGINERROR(restErrorAccountNotExist,                               901,"ACCOUNTNOTEXIST")\
    RESTLOGINERROR(restErrorTypeNotMatch,                                  909,"TYPENOTMATCH")\
    RESTLOGINERROR(restErrorNoRecordMatch,                               20004,"NORECORDMATCH")\
    RESTLOGINERROR(restErrorNucleusidPersonaidEmpty,                     20006,"NUCLEUSIDPERSONAIDEMPTY")\
    RESTLOGINERROR(restErrorFriendEmpty,                                 20007,"FRIENDEMPTY")\
    RESTLOGINERROR(restErrorPrivacySettingNotSupported,                  20008,"PRIVACYSETTINGNOTSUPPORTED")\
    RESTLOGINERROR(restErrorOutFriendLimit,                              22001,"OUTFRIENDLIMIT")\
    RESTLOGINERROR(restErrorNoGroupName,                                 22002,"NOGROUPNAME")\
    RESTLOGINERROR(restErrorGlobalGroupForbidden,                        22003,"GLOBALGROUPFORBIDDEN")\
    RESTLOGINERROR(restErrorRequestAuthtokenInvalid,                     22004,"REQUESTAUTHTOKENINVALID")\
    RESTLOGINERROR(restErrorUserConsistencyErrorCode,                    22005,"USERCONSISTENCYERRORCODE")\
    RESTLOGINERROR(restErrorInviterNotInInviteeRequestlist,              22006,"INVITERNOTININVITEEREQUESTLIST")\
    RESTLOGINERROR(restErrorDeleteUserNotInFriendlist,                   22007,"DELETEUSERNOTINFRIENDLIST")\
    RESTLOGINERROR(restErrorUserCannotDeleteFriendInNormalGroup,         22008,"USERCANNOTDELETEFRIENDINNORMALGROUP")\
    RESTLOGINERROR(restErrorInviterInInviteeBlocklist,                   22009,"INVITERININVITEEBLOCKLIST")\
    RESTLOGINERROR(restErrorInviteeInInviterFriendlist ,                 22010,"INVITEEININVITERFRIENDLIST")\
    RESTLOGINERROR(restErrorInviteeInInviterBlocklist,                   22011,"INVITEEININVITERBLOCKLIST")\
    RESTLOGINERROR(restErrorPresenceNotEmpty,                            22012,"PRESENCENOTEMPTY")\
    RESTLOGINERROR(restErrorFriendNotInGroup,                            22013,"FRIENDNOTINGROUP")\
    RESTLOGINERROR(restErrorBatchAddFriendsError,                        22014,"BATCHADDFRIENDSERROR")\
    RESTLOGINERROR(restErrorBlockUserself,                               22015,"BLOCKUSERSELF")\
    RESTLOGINERROR(restErrorInviteeIsInviterHimself,                     22017,"INVITEEISINVITERHIMSELF")\
    RESTLOGINERROR(restErrorLoginServerError,                            24001,"LOGINSERVERERROR")\
    RESTLOGINERROR(restErrorNucleusidServerError,                        24002,"NUCLEUSIDSERVERERROR")\
    RESTLOGINERROR(restErrorCassandraAccessError,                        24003,"CASSANDRAACCESSERROR")\
    RESTLOGINERROR(restErrorMysqlAccessError,                            24004,"MYSQLACCESSERROR")\
    RESTLOGINERROR(restErrorParamterTooLong,                             10103,"PARAMTERTOOLONG")\
    RESTLOGINERROR(restErrorGalleryInfoEmpty,                            10113,"GALLERYINFOEMPTY")\
    RESTLOGINERROR(restErrorUserIdsEmpty,                                10118,"USERIDSEMPTY")\
    RESTLOGINERROR(restErrorNoDownloadUrl,                               15004,"NO_DOWNLOAD_URL")\
    RESTLOGINERROR(restErrorAuthentication,                              10000, "ERROR_AUTHENTICATION")\
    RESTLOGINERROR(restErrorAuthenticationNucleus,                       10001, "ERROR_AUTHENTICATION_NUCLEUS")\
    RESTLOGINERROR(restErrorAuthenticationCredentials,                   10002, "ERROR_AUTHENTICATION_CREDENTIALS")\
    RESTLOGINERROR(restErrorAuthenticationRememberMe,                    10003, "ERROR_AUTHENTICATION_REMEMBERME")\
    RESTLOGINERROR(restErrorInvalidCaptcha,                              10004, "ERROR_INVALID_CAPTCHA")\
    RESTLOGINERROR(restErrorAuthenticationHttp,                          10005, "ERROR_AUTHENTICATION_HTTP")\
    RESTLOGINERROR(restErrorAuthenticationSSO,                           10006, "ERROR_AUTHENTICATION_SSO")\
    RESTLOGINERROR(restErrorAuthenticationOfflineAppCache,               10007, "ERROR_AUTHENTICATION_APPCACHE")\
    RESTLOGINERROR(restErrorAuthenticationClient,                        10008, "ERROR_AUTHENTICATION_CLIENT")\
    RESTLOGINERROR(restErrorAuthenticationClientRememberMe,              10009, "ERROR_AUTHENTICATION_CLIENT_REMEMBERME")\
    RESTLOGINERROR(restErrorAuthenticatinClientTFARememberMachine,       10010, "ERROR_AUTHENTICATION_CLIENT_TFA_REMEMBER_MACHINE")\
    RESTLOGINERROR(restErrorRequestAuthtokenInvalidLoginRegistration,    10044, "ERROR_REQUEST_AUTHTOKEN_INVALID")\
    RESTLOGINERROR(restErrorTokenExpired,                                10061, "ERROR_TOKEN_EXPIRED")\
    RESTLOGINERROR(restErrorTokenInvalid,                                10062, "ERROR_TOKEN_INVALID")\
    RESTLOGINERROR(restErrorTokenUseridDifferLoginRegistration,          10066, "ERROR_TOKEN_USERID_DIFFER")\
    RESTLOGINERROR(restErrorAuthenticationOfflineAppCacheMissing,        10099, "ERROR_AUTHENTICATION_APPCACHEMISSING")\
    RESTLOGINERROR(restErrorAuthenticationOfflineAppCacheCorrupt,        10100, "ERROR_AUTHENTICATION_APPCACHECORRUPT")\
    RESTLOGINERROR(restErrorAuthenticationOfflineUrlMissing,             10101, "ERROR_AUTHENTICATION_OFFLINEURLMISSING")\
    RESTLOGINERROR(restErrorLoginPageLoad,                               10104, "ERROR_LOGINPAGELOAD_FAILURE")\
    RESTLOGINERROR(restErrorOfflineCacheTransfer,                        10105, "ERROR_OFFLINE_CACHE_TRANSFER")\
    RESTLOGINERROR(restErrorLoginRedirectLoop,                           10116, "ERROR_LOGINPAGE_REDIRECT_LOOP")\
    RESTLOGINERROR(restErrorParentForeignKeyConstraintLoginRegistration, 10119, "ERROR_PARENT_FOREIGN_KEY_CONSTRAINT")\
    RESTLOGINERROR(restErrorChildForeignKeyConstraintLoginRegistration,  10120, "ERROR_CHILD_FOREIGN_KEY_CONSTRAINT")\
    RESTLOGINERROR(restErrorDuplicateEntryLoginRegistration,             10121, "ERROR_DUPLICATE_ENTRY")\
    RESTLOGINERROR(restErrorNoSuchReferenceType,                         10122, "ERROR_NO_SUCH_REFERENCE_TYPE")\
    RESTLOGINERROR(restErrorEmailNotConsistent,                          10123, "ERROR_EMAIL_NOT_CONSISTENT")\
    RESTLOGINERROR(restErrorRegistrationSourceIllegal,                   10124, "ERROR_REGISTRATIONSOURCE_ILLEGAL")\
    RESTLOGINERROR(restErrorDateOfBirthInvalid,                          10125, "ERROR_DATE_OF_BIRTH_INVALID")\
    RESTLOGINERROR(restErrorRegistrationSourceUnrecognized,              10126, "ERROR_REGISTRATIONSOURCE_UNRECOGNIZED")\
    RESTLOGINERROR(restErrorMissingEmailOrEaLoginId,                     10127, "ERROR_MISSING_EMAIL_OR_EALOGINID")\
    RESTLOGINERROR(restErrorWeakPassword,                                10128, "ERROR_WEAK_PASSWORD")\
    RESTLOGINERROR(restErrorMissingEaLoginId,                            10129, "ERROR_MISSING_EALOGINID")\
    RESTLOGINERROR(restErrorAccessBlaze,                                 10130, "ERROR_ACCESS_BLAZE")\
    RESTLOGINERROR(restErrorCredentialDoesNotExist,                      10131, "ERROR_CREDENTIAL_NOT_EXIST")\
    RESTLOGINERROR(restErrorBlazeIdInvalid,                              10132, "ERROR_BLAZE_ID_INVALID")\
    RESTLOGINERROR(restErrorFriendsIdMissing,                            10133, "ERROR_FRIENDS_ID_MISSING")\
    RESTLOGINERROR(restErrorMustChangePassword,                          10134, "ERROR_MUST_CHANGE_PASSWORD")\
    RESTLOGINERROR(restErrorVoiceServerUnderMaintenance,                 10141, "ERROR_VOICE_SERVER_UNDER_MAINTENANCE")\
    RESTLOGINERROR(restErrorMissingXOriginUid,                           10203, "ERROR_MISSING_X_ORIGIN_UID")\
    RESTLOGINERROR(restErrorXOriginUidChecksumMismatch,                  10204, "ERROR_X_ORIGIN_UID_CHECKSUM_MISMATCH")\
    RESTLOGINERROR(restErrorInvalidMachineHash,                          10205, "ERROR_INVALID_MACHINE_HASH")\
    RESTLOGINERROR(restErrorNucleusSecurityStateRed,                     10206, "ERROR_NUCLEUS_SECURITY_STATE_RED")\
    RESTLOGINERROR(restErrorForbidden,                                   10207, "ERROR_FORBIDDEN")\
    RESTLOGINERROR(restErrorUserNotInTrial,                              10209, "ERROR_USER_NOT_IN_TRIAL")\
    RESTLOGINERROR(restErrorRs4Exception,                                11000, "ERROR_RS4_EXCEPTION")\
    RESTLOGINERROR(restErrorNucleusTimeout,                              11001, "ERROR_NUCLEUS_TIMEOUT")\
    RESTLOGINERROR(restErrorIdentitySetUserProfile,                      30000, "ERROR_IDENTITYSETUSERPROFILE")\
    RESTLOGINERROR(restErrorIdentitySetUserPersona,                      30001, "ERROR_IDENTITYSETUSERPERSONA")\
    RESTLOGINERROR(restErrorIdentityRefreshTokens,                       30002, "ERROR_IDENTITYREFRESHTOKENS")\
    RESTLOGINERROR(restErrorIdentityGetTokenInfo,                        30003, "ERROR_IDENTITYGETTOKENINFO")\
    RESTLOGINERROR(restErrorAtomSetAppSettings,                          30004, "ERROR_ATOMSETAPPSETTINGS")\
    RESTLOGINERROR(restErrorSessionOfflineAuthFailed,                    1401, "ERROR_SESSION_OFFLINE_AUTH_FAILED")\
    RESTLOGINERROR(restErrorSessionOfflineAuthUnavailable,               1400, "ERROR_SESSION_OFFLINE_AUTH_UNAVAILABLE")\
    RESTLOGINERROR(restErrorNucleus2InvalidRequest,                      1300,  "ERROR_IDENTITY_INVALID_REQUEST")\
    RESTLOGINERROR(restErrorNucleus2InvalidClient,                       1301,  "ERROR_IDENTITY_INVALID_CLIENT")\
    RESTLOGINERROR(restErrorNucleus2InvalidGrant,                        1302,  "ERROR_IDENTITY_INVALID_GRANT")\
    RESTLOGINERROR(restErrorNucleus2InvalidScope,                        1310, "ERROR_IDENTITY_INVALID__SCOPE")\
    RESTLOGINERROR(restErrorNucleus2InvalidToken,                        1311, "ERROR_IDENTITY_INVALID_TOKEN")\
    RESTLOGINERROR(restErrorNucleus2UnauthorizedClient,                  1320,  "ERROR_IDENTITY_UNAUTHORIZED_CLIENT")\
    RESTLOGINERROR(restErrorNucleus2UnsupportedGrantType,                1321,  "ERROR_IDENTITY_UNSUPPORTED_GRANT_TYPE")\
    RESTLOGINERROR(restErrorIdentityGetTokens,                           1203, "ERROR_IDENTITY_GET_TOKENS")\
    RESTLOGINERROR(restErrorIdentityGetUserProfile,                      1202, "ERROR_IDENTITY_GET_USER_PROFILE")\
    RESTLOGINERROR(restErrorIdentityGetUserPersona,                      1201, "ERROR_IDENTITY_GET_USER_PROFILE")\
    RESTLOGINERROR(restErrorIdentityGetUserPid,                          1200, "ERROR_IDENTITY_GET_USER_PID")\
    RESTLOGINERROR(restErrorAtomGetAppSettings,                          1100, "ERROR_ATOM_GET_APP_SETTINGS")

    
    // The above was added for Mac Alpha Trial error checking 11/26/2012

namespace Origin
{
    namespace Services
    {
        enum restError
        {
            #define RESTLOGINERROR(enumid, enumvalue, errorstring) enumid = enumvalue,
                RESTLOGINERRORS
            #undef  RESTLOGINERROR
        };

        namespace ApiValues
        {


            //////////////////////////////////////////////////////////////////////////
            // VISIBILITY MAP

            ///
            /// Used to express a user's profile visibility setting
            ///
            enum visibility
            {
                ///
                /// Visibility default
                ///
                visibilityWrong,
                ///
                /// Visible to everyone
                ///
                visibilityEveryone,

                ///
                /// Visible to direct friends and friends of friends
                ///
                visibilityFriendsOfFriends,

                ///
                /// Visible only to direct friends
                ///
                visibilityFriends,

                ///
                /// Visible to nobody except the user
                ///
                visibilityNoOne
            };

            ///////////////////////////
            // IMAGE TYPES
            enum imageType
            {
                ImageTypeJpeg
                ,ImageTypeBmp
                ,ImageTypePng
                ,ImageTypeGif
                ,ImageTypeNoType
            };

            ///////////////////////////
            // IMAGE STATUSES
            // Utilized by Avatar and Gallery
            enum imageStatus
            {
                ImageStatusEditing = 1
                ,ImageStatusApproved
                ,ImageStatusAll
                ,ImageStatusNoStatus
            };

            ///////////////////////////
            // AVATAR TYPES
            // Utilized by Avatar and Gallery
            enum avatarType
            {
                AvatarTypeNormal = 1
                ,AvatarTypeOfficial
                ,AvatarTypePremium
                ,AvatarTypeElite
                ,AvatarTypeNoType
            };

            //////////////////////////////////////////////////////////////////////////
            // SEARCH OPTIONS
            // Utilized elsewhere and in Login registration
            enum searchOptionType
            {
                SearchOptionTypeXbox,
                SearchOptionTypePS3,
                SearchOptionTypeFaceBook,
                SearchOptionTypeFullName,
                SearchOptionTypeGamerName,
                SearchOptionTypeEmail,
                SearchOptionTypeNoOption
            };

            //////////////////////////////////////////////////////////////////////////
            /// PRIVACY SETTINGS CATEGORIES
            ///
            enum privacySettingCategory
            {
                PrivacySettingCategoryNoCategory
                ,PrivacySettingCategoryAll
                ,PrivacySettingCategoryAccount
                ,PrivacySettingCategoryGeneral
                ,PrivacySettingCategoryNotification
                ,PrivacySettingCategoryPrivacy
                ,PrivacySettingCategoryInGame
                ,PrivacySettingCategoryHiddenGames
                ,PrivacySettingCategoryFavoriteGames
                ,PrivacySettingCategoryTelemetryOptOut
                ,PrivacySettingCategoryEnableIGO
                ,PrivacySettingCategoryEnableCloudSaving
                ,PrivacySettingCategoryBroadcasting
            };

            //////////////////////////////////////////////////////////////////////////
            /// User Profile Response Levels
            ///
            enum profileResponseType
            {
                ProfileResponseTypeFull,
                ProfileResponseTypeMin
            };
        }

        enum HttpVerb
        {
            HttpInvalid
            ,HttpGet
            ,HttpPut
            ,HttpPost
            ,HttpDelete
        };

    }
}


#endif
