#ifndef LOCALHOST_ISERVICE_H
#define LOCALHOST_ISERVICE_H

#include <QObject>
#include <QMap>
#include <QNetworkCookie>
#include <QUrl>

#include "../HttpStatusCodes.h"
#include "services/common/JsonUtil.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            struct ResponseInfo 
            {
                QString response;
                int errorCode;

            };

            enum 
            {
                SECURE_SERVICE_BIT_PINGSERVER = 0
            };

            class IService : public QObject
            {
                Q_OBJECT

                public:
                    IService(QObject *parent):QObject(parent), mMethod("GET"){};
                    virtual ~IService(){};

                    ///
                    /// method returns with this service is accessed via a "GET" or "POST" call
                    ///
                    const QString &method() { return mMethod; }

                    ///
                    /// processRequest is where all the logic goes to get information from Origin that the service requires
                    ///
                    virtual ResponseInfo processRequest(QUrl requestUrl) = 0;

                    ///
                    /// isAsynchronousService returns whether or not this service is to be handled asynchronously
                    ///
                    virtual bool isAsynchronousService() { return false; }

                    ///
                    /// isSecureService returns whether or not this service is needs security handshake first
                    ///
                    virtual bool isSecureService() { return false; }

                    ///
                    /// getSecurityBitPosition returns bit position index of this service's security mask
                    ///
                    virtual int getSecurityBitPosition() { return -1; }

                signals:
                    void processCompleted(ResponseInfo);

                protected:
                    QString mMethod;

            };
        }
    }
}

#endif