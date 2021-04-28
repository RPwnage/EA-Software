#ifndef LOCALHOST_AUTHREQSTATUSSERVICE_H
#define LOCALHOST_AUTHREQSTATUSSERVICE_H

#include "LocalHost/LocalHostServices/IService.h"
#include "engine/login/LoginController.h"
#include "AuthenticationRequestStartService.h"
#include "AuthenticationRequestStartAuthcodeService.h"
#include <QTimer>
#include <QReadWriteLock>

namespace Origin
{
    namespace Sdk 
    {
        namespace LocalHost
        {
            class AuthenticationRequestStatusService : public IService
            {
                Q_OBJECT

                friend AuthenticationRequestStartService;
                friend AuthenticationRequestStartAuthcodeService;

                public:
                    AuthenticationRequestStatusService(QObject *parent);
                    ~AuthenticationRequestStatusService();

                    virtual ResponseInfo processRequest(QUrl requestUrl);

                protected:
                    enum 
                    {
                        AuthStateSSOUninitialized,
                        AuthStateSSOInProgress,
                        AuthStateSSOLoggedIn,
                        AuthStateSSOFailed,
                        AuthStateSSOFailedCannotCompareUser,
                    };


                    void setAuthState(int authState);
                    QString authStateToStringLookup(int authState);

                    void authStarted(QString uuid);

                    void startListenForAuthStateChanges();
                    void stopListenForAuthStateChanges();

                    QString mCurrentLoginType;
                    QString mAuthUUID;
                    int mAuthState;

                    QReadWriteLock mAuthStateLock;
                protected slots:
                    void onLoginError();
                    void onSSOLoggedIn();
                    void onTypeOfLogin(QString loginType);
                    void onSSOIdentifyError();
            };
        }
    }
}

#endif