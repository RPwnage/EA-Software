//  ISession.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef ISESSION_H
#define ISESSION_H

#include "services/rest/OriginServiceValues.h"
#include "services/rest/HttpStatusCodes.h"
#include "services/plugin/PluginAPI.h"

#include <QtCore>

class QNetworkReply;

namespace Origin
{
    namespace Services
    {
        namespace Session
        {
            /// \brief Defines the various error codes returned by a session.
            enum ErrorCode
            {
                SESSION_NO_ERROR,
                SESSION_NETWORK_ERROR,
                SESSION_CREDENTIAL_AUTHENTICATION_FAILED,
                SESSION_TOKEN_AUTHENTICATION_FAILED,
                SESSION_UNABLE_TO_AUTOLOGIN,
                SESSION_OFFLINE_AUTHENTICATION_FAILED,   //trying to log in via offline cache (network connection failure), offline cache exists, but unable to decrypt
                SESSION_OFFLINE_AUTHENTICATION_UNAVAILABLE, //network connection failure but no offline cache exists
                SESSION_MUST_CHANGE_PASSWORD,

                SESSION_EXPIRED,
                SESSION_FAILED_UNKNOWN,
                SESSION_FAILED_OLC_NOT_FOUND,        //used for detection of no OLC, if not found, treat it as an error and request credentials so OLC can be created
                SESSION_UPDATE_PENDING,               //used to indicate that update is pending, and user has chosen to go offline mode, so we need to request credentials for non-credentialled login
                SESSION_USER_NOT_IN_MAC_TRIAL,       //used to show error message if user is not in the Mac Alpha Trial

                //Nucleus 2.0 session errors    
                SESSION_FAILED_EXTRACT_CID,         //unable to decode the cid from idToken
                SESSION_FAILED_NUCLEUS,             //Nucleus did not provide an authorization code

                SESSION_FAILED_IDENTITY

            };

            /// \brief Defines a session error which is a combination of a lower-level REST
            /// service error and a higher-level session error code.
            class ORIGIN_PLUGIN_API SessionError
            {
            public:

                /// \brief Constructs SessionError object with "success" error codes.
                SessionError()
                    : mRestError(restErrorSuccess)
                    , mErrorCode(convert(restErrorSuccess))
                {
                }

                /// \brief Assigns given rest error code and constructs session error based on the REST error.
                SessionError(restError error)
                    : mRestError(error)
                    , mErrorCode(convert(error))
                {
                }

                /// \brief Constructs a restError and errorCode from an httpStatusCode
                SessionError(HttpStatusCode statusCode, ErrorCode errorCode)
                    : mRestError(convertToRest(statusCode)),
                      mErrorCode(errorCode)
                {
                }

                ///
                /// to be used only to override the session error code for non-REST-error cases like when .olc file is missing;
                ///
                void overrideSessionErrorCode (ErrorCode code)
                {
                    mRestError = restErrorSuccess;
                    mErrorCode = code;
                }

                /// \brief Implicit conversion of restError to SessionError.
                SessionError& operator=(restError error)
                {
                    mRestError = error;
                    mErrorCode = convert(error);
                    return *this;
                }

                bool operator==(ErrorCode error)
                {
                    return error == mErrorCode;
                }

                /// \return Session error code.
                ErrorCode errorCode() const
                {
                    return mErrorCode;
                }

                /// \return REST error code.
                restError restErrorCode() const
                {
                    return mRestError;
                }

            private:

                /// \return ErrorCode equivalent of given restError
                ///
                /// \note Not all restError codes have ErrorCode (session
                ///       error) equivalents.
                ErrorCode convert(restError error)
                {
                    switch (error)
                    {
                    case restErrorSuccess:
                        return SESSION_NO_ERROR;

                    case restErrorIdentityGetTokens:
                    case restErrorIdentityRefreshTokens:
                    case restErrorIdentityGetUserProfile:
                    case restErrorIdentityGetUserPersona:
                    case restErrorAtomGetAppSettings:
                    //Nucleus 2.0 authentication errors
                    case restErrorNucleus2InvalidRequest:
                    case restErrorNucleus2InvalidClient:
                    case restErrorNucleus2InvalidGrant:
                    case restErrorNucleus2InvalidScope:
                    case restErrorNucleus2InvalidToken:
                    case restErrorNucleus2UnauthorizedClient:
                    case restErrorNucleus2UnsupportedGrantType:
                        return SESSION_FAILED_IDENTITY;

                    case restErrorAuthenticationRememberMe:
                        return SESSION_CREDENTIAL_AUTHENTICATION_FAILED;
                    case restErrorRequestAuthtokenInvalidLoginRegistration:
                    case restErrorTokenExpired:
                    case restErrorTokenInvalid:
                        return SESSION_TOKEN_AUTHENTICATION_FAILED;

                    case restErrorSessionOfflineAuthFailed:
                        return SESSION_OFFLINE_AUTHENTICATION_FAILED;

                    case restErrorSessionOfflineAuthUnavailable:
                        return SESSION_OFFLINE_AUTHENTICATION_UNAVAILABLE;

                    case restErrorMustChangePassword:
                        return SESSION_MUST_CHANGE_PASSWORD;

                    case restErrorNetwork_TemporaryNetworkFailure:
                    case restErrorNetwork_SslHandshakeFailed:
                    case restErrorNetWork_OperationCanceledError:   //it will return this if we abort the request per our own timeout value
                    case restErrorNetwork_TimeoutError:
                    case restErrorNetwork_HostNotFound:
                    case restErrorNetwork_RemoteHostClosed:
                    case restErrorNetwork_ConnectionRefused:
                    case restErrorNetwork:
                        return SESSION_NETWORK_ERROR;

                    //This was added for the Mac Aplha Trial
                    case restErrorUserNotInTrial:
                            return SESSION_USER_NOT_IN_MAC_TRIAL;

                    case restErrorAuthenticationNucleus:
                        return SESSION_FAILED_NUCLEUS;

                    default:
                        return SESSION_FAILED_UNKNOWN;
                    }
                }

                restError convertToRest(HttpStatusCode statusCode)
                {
                    switch(statusCode)
                    {
                        case HttpSuccess:
                        case Http200Ok:
                        case Http201Created:
                        case Http202Accepted:
                        case Http203NonAuthoritativeInfo:
                        case Http204NoContent:
                        case Http205ResetContent:
                        case Http206PartialContent:

                            return restErrorSuccess;

                        case Http300MultipleChoices:
                        case Http301MovedPermanently:
                        case Http302Found:
                        case Http303SeeOther:
                        case Http304NotModified:
                        case Http305UseProxy:
                        case Http306Unused:
                        case Http307TemporaryRedirect:
                        case Http400ClientErrorBadRequest:
                        case Http401ClientErrorUnauthorized:
                        case Http402ClientErrorPaymentRequired:
                        case Http403ClientErrorForbidden:
                        case Http404ClientErrorNotFound:
                        case Http405ClientErrorMethodNotAllowed:
                        case Http406ClientErrorNotAcceptable:
                        case Http407ClientErrorProxyAuthenticationRequired:
                        case Http408ClientErrorRequestTimeout:
                        case Http409ClientErrorConflict:
                        case Http410ClientErrorGone:
                        case Http411ClientErrorLengthRequired:
                        case Http412ClientErrorPreconditionFailed:
                        case Http413ClientErrorRequestEntityTooLarge:
                        case Http414ClientErrorRequestURITooLong:
                        case Http415ClientErrorUnsupportedMediaType:
                        case Http416ClientErrorRequestedRangeNotSatisfiable:
                        case Http417ClientErrorExpectationFailed:

                            return restErrorUnknown;

                        case Http500InternalServerError:
                        case Http501NotImplemented:
                        case Http502BadGateway:
                        case Http503ServiceUnavailable:
                        case Http505HTPVersionNotSupported:

                            return restErrorNetwork_ConnectionRefused;

                        case Http504GatewayTimeout:
                            return restErrorNetwork_TimeoutError;

                        default:
                            return restErrorUnknown;
                    }

                }

                /// \brief REST error code
                restError mRestError;

                /// \brief Session error code.
                ///        The session error code is always assigned based on
                ///        the value of the REST error code in the constructor.
                ///
                /// \sa    convert()
                ErrorCode mErrorCode;
            };

            enum LoginOptionBits
            {
                LOGIN_MANUAL    = 0,
                LOGIN_AUTOMATIC = 1,
                LOGIN_VISIBLE   = 0,
                LOGIN_INVISIBLE = 2
            };
            typedef int LoginOptions;

            class ORIGIN_PLUGIN_API SessionToken : public QString
            {
            public:
                SessionToken() : QString() {}
                SessionToken(SessionToken const& from) : QString(from) {}
                explicit SessionToken(QString const& from) : QString(from) {}
            };

            class ORIGIN_PLUGIN_API SsoToken : public SessionToken
            {
            public:
                SsoToken() : SessionToken() {}
                SsoToken(SsoToken const& from) : SessionToken(from) {}
                explicit SsoToken(QString const& from) : SessionToken(from) {}
            };

            class ORIGIN_PLUGIN_API AccessToken : public SessionToken
            {
            public:
                AccessToken() : SessionToken() {}
                AccessToken(AccessToken const& from) : SessionToken(from) {}
                explicit AccessToken(QString const& from) : SessionToken(from) {}
            };

            class ORIGIN_PLUGIN_API RefreshToken : public SessionToken
            {
            public:
                RefreshToken() : SessionToken() {}
                RefreshToken(RefreshToken const& from) : SessionToken(from) {}
                explicit RefreshToken(QString const& from) : SessionToken(from) {}
            };

            class AbstractSession;
            typedef QSharedPointer<AbstractSession> SessionRef;
            typedef QWeakPointer<AbstractSession> SessionWRef;

            class ORIGIN_PLUGIN_API AbstractSessionConfiguration
            {
            public:

                virtual ~AbstractSessionConfiguration() {}
            };

            /// \brief Defines the interface for all sessions. 
            class ORIGIN_PLUGIN_API AbstractSession : public QObject
            {
                friend class SessionService;

                Q_OBJECT

            public:

                virtual ~AbstractSession() {}

                /// \brief Base class to be used with service-specific per session storage
                struct ServiceData 
                {
                    virtual ~ServiceData(){}
                };
                typedef QSharedPointer<ServiceData> ServiceDataRef;

                /// \brief Return the service specific storage for the given key
                ServiceDataRef serviceData(QString const& key)                  { return mServiceData[key]; }

                /// \brief Add service specific storage under the given key
                void addServiceData(QString const& key, ServiceDataRef value)   { mServiceData[key] = value; }

                /// \brief Remove the service specific storage for the given key
                void removeServiceData(QString const& key)                      { mServiceData.remove(key); }

                /// \brief renew token
                virtual void renewAccessToken()=0;

            protected:
                AbstractSession() {}

                /// \brief Return a shared pointer to this, Qt doesn't have enable_shared_from_this
                SessionRef self()                                               { return mSelf.toStrongRef(); }

                /// \brief Set the shared pointer do this, call from factory function
                void setSelf(SessionRef self)                                   { mSelf = self; }

                /// \brief Initiates a session using the given parameters.
                virtual void beginAsync(AbstractSessionConfiguration const& sessionConfiguration) = 0;

                /// \brief Terminates the session.
                virtual void endAsync() = 0;

                /// \brief Returns a reference to the session's configuration.
                virtual AbstractSessionConfiguration const& configuration() const = 0;

            signals:

                /// \brief Signal triggered when the matching beginAsync() calls has completed.
                void beginFinished(SessionError error, SessionRef session);

                /// \brief Signal triggered when the matching endAsync() calls has completed.
                void endFinished(SessionError error, SessionRef session);

                /// \brief Signals when a session has ended.
                /// TODO this is never emitted
                void sessionEnded(SessionError error, SessionRef session);

            protected slots:

                virtual void onBeginFinished() = 0;
                virtual void onEndFinished() = 0;

            private:
                SessionWRef mSelf;
                QMap<QString, ServiceDataRef> mServiceData;
            };

            typedef SessionRef (*SessionFactory)();

        } // namespace Session

        /// Return type from request calls to authenticated service clients. Since
        /// smart pointers should always be passed by value, this type should 
        /// also always be passed by value.
        typedef QPair<QNetworkReply*, Session::SessionWRef> AuthNetworkRequest;

    } // namespace Services
} // namespace Origin

#endif // ISESSION_H
