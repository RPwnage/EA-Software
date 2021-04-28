#ifndef LOCALHOSTSECURITY_H
#define LOCALHOSTSECURITY_H

#include <QString>
#include <QMap>
#include <QObject>
#include <QHostAddress>
#include <QTimer>
#include <QSslError>

#include "services/network/NetworkAccessManager.h"


namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            typedef struct
            {
                uint32_t mServiceMask;      // services currently secured
                uint32_t mTimeRemaining;    // for how long
            } SecurityStatusType;

            class LocalHostSecurityRequest:public QObject
            {
                Q_OBJECT

            public:
                LocalHostSecurityRequest(QObject *parent, QString domain):mDomain(domain) {}
                ~LocalHostSecurityRequest() {}

            signals:
                void validationCompleted(bool);
                void closeRequest(LocalHostSecurityRequest *);

            protected slots:
                void onEncrypted();
                void onErrorReceived(QNetworkReply::NetworkError err);

            public:
                QNetworkReply *mNetworkReply;
                QString mDomain;
            };

            class LocalHostSecurity:public QObject
            {
                Q_OBJECT

                public:

                    static LocalHostSecurity *instance();
                    static void start();
                    static void stop();
                    static bool isCurrentlyUnlocked(QString domain, uint32_t service_bit);
                    static LocalHostSecurityRequest *validateConnection(QString domain);

                    void verifyServices(QString domain, QStringList service_list);

                protected slots:
                    void onTimerInterval();
                    void onCloseRequest(LocalHostSecurityRequest *);

                private:
                    void registerServices();

                    bool mVerificationDisabled;
                    Origin::Services::NetworkAccessManager *mNetworkAccessManager;
                    QTimer mUpdateTimer;
                    QMap<QString, SecurityStatusType> mSecurityStatusMap;
                    QMap<QString, int> mSecureServiceMap;
                    QList<LocalHostSecurityRequest *> mRequestList;
            };

        }
    }
}

#endif