#ifndef _ORIGINUSERPROXY_H
#define _ORIGINUSERPROXY_H

#include <QObject>
#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            class ORIGIN_PLUGIN_API OriginUserProxy : public QObject
            {
                Q_OBJECT
            public:
                OriginUserProxy(Engine::UserRef user);

                Q_PROPERTY(bool socialAllowed READ socialAllowed);
                bool socialAllowed();

                Q_PROPERTY(bool commerceAllowed READ commerceAllowed);
                bool commerceAllowed();

                Q_PROPERTY(QString originId READ originId);
                QString originId();

                Q_PROPERTY(QString nucleusId READ nucleusId);
                QString nucleusId();

                Q_PROPERTY(QString personaId READ personaId);
                QString personaId();

                Q_PROPERTY(QString dob READ dob);
                QString dob();

                Q_INVOKABLE QVariantMap offlineUserInfo();

                Q_INVOKABLE void requestLogout();

                /// \brief Async request to refresh SID cookie value.
                ///
                /// sidRenewalResponseProxy gets called with the HTTP status code 
                /// when the response completes. Note that the call may or may not
                /// be successful and the requestor needs to check the status code
                /// to determine success/fail.
                ///
                /// \sa sidRenewalResponseProxy
                Q_INVOKABLE void requestSidRefresh();

            signals:

                /// \brief Connect to this signal in JS with your callback.
                ///
                /// JS example:
                /// OriginUser.sidRenewalResponseProxy.connect(myCallBack);
                ///
                /// \param restError REST error code
                /// \param httpStatus HTTP status code
                void sidRenewalResponseProxy(qint32 restError, qint32 httpStatus);

            private:
                Engine::UserRef mUser;
            };

        }
    }
}

#endif
