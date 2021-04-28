#ifndef _LSXCONNECTION_H
#define _LSXCONNECTION_H

#include <QString>
#include <QObject>
#include <QByteArray>
#include <QAtomicInt>
#include "LsxMessages.h"
#include "engine/content/Entitlement.h"

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class Server;

            class Connection : public QObject
            {
                Q_OBJECT
            public:
                void setGameName(const QString &gameName);
                void setProductId(const QString &productId);
                void setMultiplayerId(const QString &multiplayerId);
                void setCommerceProfile(const QString &commerceProfile);
                void setSdkVersion(const QString &sdkVersion){ m_sdkVersion = sdkVersion; }
                void setSeed(int seed){ m_seed = seed; };

                void FindTitleAndProfile(QString productId, QString multiplayerId);

                QString gameName() const { return m_gameName; }
                QString productId() const { return m_productId; }
                QString multiplayerId() const { return m_multiplayerId; }
                QString commerceProfile(); 
                QString sdkVersion() const { return m_sdkVersion; } 
                int seed(){ return m_seed; }

                Origin::Engine::Content::EntitlementRef entitlement();

                int id(){ return m_Id; }

                virtual void sendResponse(const Response& response) = 0;

                // Indicates if we're still connected
                bool connected() const;

                // Reference counting support
                void ref();
                void unref();

                int CompareSDKVersion(int major, int minor, int rev_major, int rev_minor) const;

                bool blockUnsolicitedEvents() { return m_blockUnsolicitedEvents; }

                void setBlockUnsolicitedEvents(bool val) { m_blockUnsolicitedEvents = val; }

                // Signal that the connection is established.
                void signalConnected();
				
				virtual void closeConnection() = 0;

            public slots:
                // Hangs up this connection
                // This removes our implicit reference and emits hungup() but might not
                // actually delete the object until all references disappear
                void hangup();

            signals:
                /// \brief signal emitted when an SDK-enabled entitlement establishes communication with the SDK
                /// \param contentId
                void connectedToSDK(QString/*& productId*/);

                /// \brief signal emitted when an SDK-enabled entitlement disconnects from the SDK
                /// \param contentId
                void hungup(const QString& productId);

            protected:
                Connection(QObject *parent = NULL);
                virtual ~Connection() {}

                QString m_gameName;
                QString m_productId;
                QString m_multiplayerId;
                QString m_commerceProfile;
                QString m_sdkVersion;
                int m_seed;

                Origin::Engine::Content::EntitlementWRef m_entitlement; 

                int m_Id;
                static int s_Id;

                // Protects the next two
                QAtomicInt m_refs;
                bool m_connected;
                bool m_blockUnsolicitedEvents;
            };
        }
    }
}

#endif