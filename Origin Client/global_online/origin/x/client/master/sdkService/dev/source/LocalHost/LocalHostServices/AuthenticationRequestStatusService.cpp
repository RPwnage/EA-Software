#include "AuthenticationRequestStatusService.h"
#include <QNetworkCookie>
#include <QDateTime>
#include <QUuid>
#include "MainFlow.h"
#include "services/settings/SettingsManager.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    using namespace Engine;

	namespace Sdk
	{
		namespace LocalHost
		{
            AuthenticationRequestStatusService::AuthenticationRequestStatusService(QObject *parent)
                :IService(parent)
                ,mAuthState(AuthStateSSOUninitialized)
            {
            }
            
            AuthenticationRequestStatusService::~AuthenticationRequestStatusService() 
            {

            }

            QString AuthenticationRequestStatusService::authStateToStringLookup(int authState)
            {
                QString stateString;
                switch(authState)
                {
                    case AuthStateSSOUninitialized:
                        stateString = "AUTHSTATUS_UNINITIALIZED";
                        break;
                    case AuthStateSSOInProgress:
                        stateString = "AUTHSTATUS_INPROGRESS";
                        break;
                    case AuthStateSSOLoggedIn:
                        stateString = "AUTHSTATUS_LOGGEDIN";
                        break;
                    case AuthStateSSOFailed:
                        stateString = "AUTHSTATUS_FAILED";
                        break;
                    case AuthStateSSOFailedCannotCompareUser:
                        stateString = "AUTHSTATUS_FAILED_NOID";
                        break;
                }

                return stateString;
            }

            void AuthenticationRequestStatusService::startListenForAuthStateChanges()
            {
                //listen for the type of login-in that is occurring, this is a sanity check, to make sure we are only looking for sso logins
                ORIGIN_VERIFY_CONNECT(
                    Origin::Engine::LoginController::instance(), SIGNAL(typeOfLogin(QString)),
                    this, SLOT(onTypeOfLogin(QString)));

                //listen for sso login errors
                ORIGIN_VERIFY_CONNECT(
                    Origin::Client::MainFlow::instance(), SIGNAL(loginError()),
                    this, SLOT(onLoginError()));

                //listen for when the user gets passed the single login and is in the client view
                ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(singleLoginFlowFinished()),
                    this, SLOT(onSSOLoggedIn()));

                //listen for when the sso is requested and the user is already logged in
                ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(ssoLoggedIn()),
                    this, SLOT(onSSOLoggedIn()));

                //listen for the error where we cannot determine if current user is the same as the user trying to login
                ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(ssoIdentifyError()),
                    this, SLOT(onSSOIdentifyError()));

            }

            void AuthenticationRequestStatusService::stopListenForAuthStateChanges()
            {
                ORIGIN_VERIFY_DISCONNECT(
                    Origin::Engine::LoginController::instance(), SIGNAL(typeOfLogin(QString)),
                    this, SLOT(onTypeOfLogin(QString)));

                ORIGIN_VERIFY_DISCONNECT(
                    Origin::Client::MainFlow::instance(), SIGNAL(loginError()),
                    this, SLOT(onLoginError()));

                ORIGIN_VERIFY_DISCONNECT(Origin::Client::MainFlow::instance(), SIGNAL(singleLoginFlowFinished()),
                    this, SLOT(onSSOLoggedIn()));

                ORIGIN_VERIFY_DISCONNECT(Origin::Client::MainFlow::instance(), SIGNAL(ssoLoggedIn()),
                    this, SLOT(onSSOLoggedIn()));

                ORIGIN_VERIFY_DISCONNECT(Origin::Client::MainFlow::instance(), SIGNAL(ssoIdentifyError()),
                    this, SLOT(onSSOIdentifyError()));
            }

            void AuthenticationRequestStatusService::authStarted(QString uuid)
            {
                mAuthUUID = uuid;
                setAuthState(AuthStateSSOInProgress);

                //hook up signals
                startListenForAuthStateChanges();
            }

            void AuthenticationRequestStatusService::setAuthState(int authState)
            { 
                QWriteLocker lock(&mAuthStateLock);
                mAuthState = authState;
            }

            void AuthenticationRequestStatusService::onTypeOfLogin(QString loginType)
            {
                mCurrentLoginType = loginType;
            }

            void AuthenticationRequestStatusService::onLoginError()
            {
                setAuthState(AuthStateSSOFailed);
                stopListenForAuthStateChanges();
            }

            void AuthenticationRequestStatusService::onSSOIdentifyError()
            {
                setAuthState(AuthStateSSOFailedCannotCompareUser);
                stopListenForAuthStateChanges();
            }

            void AuthenticationRequestStatusService::onSSOLoggedIn()
            {
                if (mCurrentLoginType.compare("sso", Qt::CaseInsensitive) == 0)
                {
                    setAuthState(AuthStateSSOLoggedIn);
                }
                else
                {
                    setAuthState(AuthStateSSOFailed);
                }
                stopListenForAuthStateChanges();
            }

			ResponseInfo AuthenticationRequestStatusService::processRequest(QUrl requestUrl)
			{
                ResponseInfo responseInfo;
                //incoming path should look like this
                //    /authentication/requeststatus/{opcode}
                //    0 - empty
                //    1 - authentication
                //    2 - requeststatus
                //    3 - {opcode}
                QString opcode;
                const int expectedPathListSize = 4;
                const int opcodeIndex = 3;
                QStringList pathPartsList = requestUrl.path().split('/');

                if(pathPartsList.size() == expectedPathListSize)
                {
                    opcode = QUrl::fromPercentEncoding(pathPartsList[opcodeIndex].toLocal8Bit());
                }
                if(!opcode.isEmpty())
                {
                    if(opcode.compare(mAuthUUID, Qt::CaseInsensitive) == 0)
                    {
                        QReadLocker lock(&mAuthStateLock);
                        if(mAuthState == AuthStateSSOInProgress)
                            responseInfo.errorCode = HttpStatus::Accepted;
                        else
                            responseInfo.errorCode = HttpStatus::OK;
                        responseInfo.response = QString("{\"status\":\"%1\"}").arg(authStateToStringLookup(mAuthState));
                    }
                    else
                    {
                        //the uuid's don't match so a request with the op code is not allowed
                        responseInfo.errorCode = HttpStatus::Forbidden;
                    }
                }
                else
                {
                    responseInfo.errorCode = HttpStatus::NotFound;
                    responseInfo.response = "{\"error\":\"OPCODE_MISSING\"}";
                }
                return responseInfo;
			}
		}
	}
}