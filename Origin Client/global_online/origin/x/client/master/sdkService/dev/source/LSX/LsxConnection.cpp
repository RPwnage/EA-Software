#include "LSX/LsxConnection.h"

#include "engine/content/contentcontroller.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"

#include "services/debug/DebugService.h"


namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            int Connection::s_Id = 1;

            // Start with one implicit reference for the connection
            Connection::Connection(QObject *parent)
                : QObject(parent)
                , m_seed(0)
                , m_Id(s_Id++)
                , m_refs(1)
                , m_connected(true)
                , m_blockUnsolicitedEvents(true)
            {
                using namespace Origin::Engine::Content;

                if (ContentController::currentUserContentController())
                {
                    ORIGIN_VERIFY_CONNECT(this, SIGNAL(connectedToSDK(QString)),ContentController::currentUserContentController(), SLOT(onConnectedToSDK(QString)));
                    ORIGIN_VERIFY_CONNECT(this, SIGNAL(hungup(const QString&)),ContentController::currentUserContentController(), SLOT(onHungupFomSDK(const QString&)));
                }
            }

            void Connection::setGameName(const QString &gameName)
            {
                m_gameName = gameName;
            }

            void Connection::setProductId(const QString &productId)
            {
                m_productId = productId;
            }

            void Connection::setMultiplayerId(const QString &multiplayerId)
            {
                m_multiplayerId = multiplayerId;
            }

            void Connection::setCommerceProfile(const QString &commerceProfile)
            {
                m_commerceProfile = commerceProfile;
            }

            Origin::Engine::Content::EntitlementRef Connection::entitlement()
            {
                return m_entitlement.toStrongRef();
            }

            void Connection::FindTitleAndProfile(QString productId, QString multiplayerId)
            {

                if(!(Origin::Engine::LoginController::isUserLoggedIn()))
                {
                    m_commerceProfile = "unknown";
                    return;
                }

                QList<Origin::Engine::Content::EntitlementRef> content = Origin::Engine::Content::ContentController::currentUserContentController()->entitlements();

                QList<Origin::Engine::Content::EntitlementRef>::iterator match = content.end();
                QList<Origin::Engine::Content::EntitlementRef>::iterator contentIdmatch = content.end();
                QList<Origin::Engine::Content::EntitlementRef>::iterator productIdmatch = content.end();


                for(QList<Origin::Engine::Content::EntitlementRef>::iterator i = content.begin(); i != content.end(); i++)
                {
                    // Only check for the entitlement that is enabled for the current platform.
                    if((*i)->contentConfiguration()->platformEnabled())
                    {
                        if((*i)->contentConfiguration()->multiplayerId() == multiplayerId)
                        {
                            match = i;
                        }
                        // Check whether the received productId is actually a contentId
                        if((*i)->contentConfiguration()->contentId() == productId)
                        {
                            contentIdmatch = i;
                            break;
                        }
                        if((*i)->contentConfiguration()->productId() == productId)
                        {
                            productIdmatch = i;
                            break;
                        }
                    }
                }

                if(productIdmatch != content.end()) // <== Product Id matches
                {
                    m_gameName = (*productIdmatch)->contentConfiguration()->displayName();
                    m_commerceProfile = (*productIdmatch)->contentConfiguration()->commerceProfile().length()>0 ? (*productIdmatch)->contentConfiguration()->commerceProfile() : "unknown";

                    // Ignore the game provided multiplayer id, and use the one in the entitlement config.
                    m_multiplayerId = (*productIdmatch)->contentConfiguration()->multiplayerId();

                    m_entitlement = (*productIdmatch);
                }
                else if(contentIdmatch != content.end()) // <== Content Id matches
                {
                    m_gameName = (*contentIdmatch)->contentConfiguration()->displayName();
                    m_commerceProfile = (*contentIdmatch)->contentConfiguration()->commerceProfile().length()>0 ? (*contentIdmatch)->contentConfiguration()->commerceProfile() : "unknown";

                    // Ignore the game provided multiplayer id, and use the one in the entitlement config.
                    m_productId = (*contentIdmatch)->contentConfiguration()->productId();
                    m_multiplayerId = (*contentIdmatch)->contentConfiguration()->multiplayerId();

                    m_entitlement = (*contentIdmatch);
                }
                else if(match != content.end())	// <== Multiplayer Id matches
                {
                    m_productId = (*match)->contentConfiguration()->productId();
                    m_gameName = (*match)->contentConfiguration()->displayName();
                    m_commerceProfile = (*match)->contentConfiguration()->commerceProfile().length()>0 ? (*match)->contentConfiguration()->commerceProfile() : "unknown";

                    m_entitlement = (*match);
                }
                else
                {
                    m_commerceProfile = "unknown";
                }
            }

            QString Connection::commerceProfile()
            {
                if(m_commerceProfile.isEmpty() || m_commerceProfile == "unknown")
                {
                    FindTitleAndProfile(productId(), multiplayerId());
                }

                return m_commerceProfile;
            }


            void Connection::signalConnected()
            {
                emit(connectedToSDK(m_productId));
            }

            bool Connection::connected() const
            {
                return m_connected;
            }

            void Connection::ref()
            {
                m_refs.ref();
            }

            void Connection::unref()
            {
                if (!m_refs.deref())
                {
                    // We can't delete ourselves if we want to be slot-safe
                    deleteLater();
                }
            }

            void Connection::hangup()
            {
                // We're no longer connected
                m_connected = false;

                // Don't allow any unsolicited message over this connection.
                m_blockUnsolicitedEvents = true;

                // Tell anyone interested that we've hungup
                emit(hungup(m_productId));

                // Remove our implicit reference
                unref();

				// Disconnect the network connection
				closeConnection();
            }

            int Connection::CompareSDKVersion( int major, int minor, int rev_major, int rev_minor ) const
            {
                int a=0, b=0, c=0, d=0;

                if(sscanf(m_sdkVersion.toLatin1().constData(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
                {
                    // Check equal
                    if(a==major && b==minor && c==rev_major && d==rev_minor)
                        return 0;

                    if(a>major || (a==major && b>minor) || (a==major && b==minor && c>rev_major) || (a==major && b==minor && c==rev_major && d>rev_minor))
                        return 1;
                }
                return -1;
            }

        }
    }
}