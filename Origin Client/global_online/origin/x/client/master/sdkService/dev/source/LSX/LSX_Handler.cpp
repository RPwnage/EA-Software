///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: LSX_Handler.cpp
//	Main handler for LSX requests
//	
//	Author: Sergey Zavadski

#include <QThread>
#include <QMetaType>
#include <QMutexLocker>

#include "LSX/LSX_Handler.h"
#include "LSX/LSX.h"
#include "LSX/LsxServer.h"
#include "LSX/LsxConnection.h"

#include "Service/XMPPService/XMPP_ServiceArea.h"
#include "Service/UtilityService/Utility_ServiceArea.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "Service/SDKService/SDK_eCommerce_ServiceArea.h"
#include "Service/ProgressiveInstallationService/ProgressiveInstallation_ServiceArea.h"

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "services/crypto/SimpleEncryption.h"
#include "services/crypto/md5.h"

#undef EAID

#include "lsx.h"
#include "lsxreader.h"
#include "lsxwriter.h"

#include "EAStdC/EAString.h"
#include "EAStdC/EASprintf.h"

#include "cryptssc2.h"

#include "version/version.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

// telemetry
#include "TelemetryAPIDLL.h"

#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/utilities/ContentUtils.h"

#if defined(ORIGIN_MAC)
#include <time.h>
#include <sys/time.h>
#include <mach/clock.h>
#include <mach/mach.h>
#endif

using Origin::Engine::Content::ContentController;

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            LSX_Handler::LSX_Handler()
                :	m_server( NULL )
            {
                //
                //  create our LSX server
                //
                m_server = new Lsx::Server(this);

                // Notify the ContentController when an SDK connection is hung up
                ORIGIN_VERIFY_CONNECT(m_server, SIGNAL(connectionHungup(const QString&)), this, SLOT(onConnectionHangup(const QString&)));

                //
                //	create service areas
                //	
                startServiceAreas();

                //
                //	start listening - this has to be done after the service areas, otherwise we can
                //  receive requests, but the appropriate service area is not yet ready and this will result in 
                //  no response from the middleware
                m_server->listen();

                //
                //	check settings and perform any necessary actions
                //
                checkSettings();
            }

            LSX_Handler::~LSX_Handler()
            {
                ORIGIN_LOG_EVENT << "EALS Shutdown";

                //
                //	stop listening for new connections
                //	
                if(m_server)
                    m_server->shutdown();

                //
                //	delete service areas
                //
                stopServiceAreas();


                ORIGIN_LOG_EVENT << "Destroy server";

                if(m_server)
                    delete m_server;
                m_server=NULL;

                ORIGIN_LOG_EVENT << "Destroy server done";
            }

            // Process an incoming request
            // This is thread safe once Middleware has started
            void LSX_Handler::processRequest( Lsx::Request * request )
            {
                ORIGIN_LOG_DEBUG << "Processing request: " << request->document()->Print();
                //
                //	Is request valid?
                //
                if ( !request->valid() )
                {
                    ORIGIN_LOG_ERROR << "SDK Request is invalid: " << request->document()->Print();
                    // Signal an error back to the SDK.

                    lsx::ErrorSuccessT err;

                    err.Code = EBISU_ERROR_LSX_INVALID_REQUEST;
                    err.Description = "The LSX message couldn't be read.";


                    Lsx::Response response(*request);
                    lsx::Write(response.document(), err);

                    return;
                }

                // We received a message from the OriginSDK indicating that a secure connection is established. Allow unsolicited events to the OriginSDK.
                if (request->connection() != NULL)
                {
                    request->connection()->setBlockUnsolicitedEvents(false);
                }    

                ORIGIN_LOG_DEBUG << "Recipient: " << request->recipient() << ", command: " << request->command();
                //
                //	Find service area
                //
                QHash< QString, BaseService* >::ConstIterator i = m_serviceAreas.find( request->recipient() );
                if ( i != m_serviceAreas.end() )
                {
                    //
                    //	Dispatch request to service area
                    //	
                    (*i)->processRequestAsync( request );
                }
            }

            void LSX_Handler::stopServiceAreas()
            {
                ORIGIN_LOG_EVENT << "StopServiceAreas";

                //
                //	Delete service areas
                //
                for ( QHash< QString, BaseService* >::ConstIterator i = m_serviceAreas.begin(); i != m_serviceAreas.end(); i++ )
                {
                    // Stop our worker thread before calling the destructor
                    // This generally means there's no requests/timers etc in flight when
                    // the destructor is called
                    (*i)->stopWorker();
                    (*i)->destroy();
                }

                m_serviceAreas.clear();
            }

            int getTimeInMs()
            {
#if defined(ORIGIN_PC)
                return GetTickCount();
#elif defined(ORIGIN_MAC)
                clock_serv_t cclock;
                host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
                mach_timespec_t mts;
                clock_get_time(cclock, &mts);
                mach_port_deallocate(mach_task_self(), cclock);
                return mts.tv_nsec / 1000000; // nsec => millisec
#else
#error "need implementation"
#endif
            }

            void LSX_Handler::clientAuthenticationChallenge( Lsx::Response& response, QByteArray &key )
            {
                int currentMillis = getTimeInMs();
                md5_state_t md5state;
                md5_init( &md5state );
                md5_append( &md5state, ( const md5_byte_t* )&currentMillis, ( int ) sizeof(currentMillis) );

                char digest[16];
                md5_finish( &md5state, ( md5_byte_t* ) digest );

                key = QByteArray(digest, 16).toHex();

				lsx::ChallengeT challenge;

				challenge.key = key;
				challenge.version = EBISU_VERSION_STR;

#ifdef _DEBUG
                challenge.build = "debug"; 
#else
                challenge.build = "release";
#endif

                ORIGIN_LOG_DEBUG << "Issuing challenge key: " << key;
                
				lsx::Write(response.document(), challenge);
				
                response.setSender(  SYSTEM_SERVICE_AREA_NAME );
            }

            bool LSX_Handler::authenticateClient( const lsx::ChallengeResponseT& challengeResponse, Lsx::Connection *connection, const QByteArray & key, Lsx::Response& serverResponse )
            {
				ORIGIN_LOG_DEBUG << "Origin challenge key: " << key;

                std::string decryptedKey;
                Services::Crypto::SimpleEncryption encryption;

                if(key.length()>0)
                {
                    decryptedKey = encryption.decrypt( challengeResponse.response.toStdString() );
                }

                if(key.length() == 0 || decryptedKey == key.data())
                {
                    // Try to pull out our game identification values
                    if(!extractGameInformationToConnection(challengeResponse, connection))
                        return false;

                    //
                    //	Authentication succeeded
                    //
                    ORIGIN_LOG_DEBUG << "Authentication succeeded";

                    if(key.length() > 0)
                    {
                        lsx::ChallengeAcceptedT accept;
                        accept.response = encryption.encrypt(challengeResponse.key.toStdString()).c_str();

                        lsx::Write(serverResponse.document(), accept);

                        // Base the encryption seed on the SDK Protocol Version
                        if(challengeResponse.ProtocolVersion == "2")
                        {
                            connection->setSeed(0);
                        }
                        else if(challengeResponse.ProtocolVersion == "3")
                        {
                            connection->setSeed(((int)(accept.response[0].toLatin1()) << 8) + accept.response[1].toLatin1());
                        }
                    }
                    else
                    {
                        lsx::ErrorSuccessT resp;
                        resp.Code = EBISU_SUCCESS;
                        resp.Description = "Successfully made a connection";

                        lsx::Write(serverResponse.document(), resp);
                    }
                    return true;
                }

                ORIGIN_LOG_DEBUG << "Authentication failed";
                return false;
            }

            void LSX_Handler::onConnectionHangup(const QString& productId)
            {
                // check if ContentController exists at all (user might not have logged in yet)
                Origin::Engine::Content::ContentController *cc = ContentController::currentUserContentController();
                if (cc != NULL)
                    QMetaObject::invokeMethod(cc, "sdkConnectionLost", Qt::QueuedConnection, Q_ARG(const QString &, productId));
            }

            bool LSX_Handler::extractGameInformationToConnection(const lsx::ChallengeResponseT &resp, Lsx::Connection *connection)
            {
                // hack to fix FIFA12 multiplayer invite not being received
                QString productid = resp.ContentId;
                if(productid == "EA203101A0X11")
                    productid = "71055"; // hack in the correct content id for FIFA12
                connection->setProductId(productid);

                connection->setMultiplayerId(resp.MultiplayerId);
                connection->setGameName(resp.Title);

                // Lets see if we can find the actual title and profile for the game
                connection->FindTitleAndProfile(connection->productId(), connection->multiplayerId());

                QString version;

                if(resp.SdkVersion.length())
                {
                    version = resp.SdkVersion;
                }
                else
                {
                    version = "pre-1.0.0.7";
                }

                connection->setSdkVersion(version);

                Origin::Engine::Content::EntitlementRef entitlement = connection->entitlement();

                ORIGIN_LOG_EVENT << "OriginSDK Version " << version << " Connected.";
                ORIGIN_LOG_EVENT << "OriginSDK Game Connected: GameName: " << connection->gameName() << ", ProductId: " << connection->productId() << ", MultiplayerId: " << connection->multiplayerId() << ", CommerceProfile:" << connection->commerceProfile() << ", Found Matching Entitlement: " << (entitlement.isNull() ? "No" : "Yes");


                QString offerId = !entitlement.isNull() ? entitlement->contentConfiguration()->productId() : connection->productId() + "-DISCONNECTED";
                GetTelemetryInterface()->Metric_ORIGIN_SDK_VERSION_CONNECTED(offerId.toLatin1().data(), version.toLatin1().data());

                if(!entitlement.isNull() && entitlement->contentConfiguration()->status() == Origin::Services::Publishing::ACTIVE)
                {
                    ORIGIN_LOG_ERROR_IF(entitlement->contentConfiguration()->multiplayerId().isEmpty() && !connection->multiplayerId().isEmpty()) << "SDK Game: The game has no multiplayer Id specified on the offer!";
                }
                else
                {
                    ORIGIN_LOG_ERROR << "SDK Game doesn't have a matching entitlement. Redeem a code for the game for full SDK support";

                    // Only hangup the connection when running on production.
                    if(Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName).toString().toLower() == "production" || 
                       Origin::Services::readSetting(Origin::Services::SETTING_DisconnectSDKWhenNoEntitlement))
                    {
                        connection->hangup();
                        return false;
                    }
                }
                
                return true;
            }

            void LSX_Handler::checkSettings()
            {
            }

            void LSX_Handler::startServiceAreas()
            {
                if ( !m_serviceAreas.size() )
                {
                    //
                    //	XMPP
                    //
                    XMPP_ServiceArea* xmppService = XMPP_ServiceArea::create(this);
                    m_serviceAreas.insert(xmppService->name(), xmppService);
                    ORIGIN_LOG_EVENT << "Started XMPP Service area";

                    //
                    //	Utility 
                    //
                    Utility_ServiceArea* utilityService = Utility_ServiceArea::create(this);
                    m_serviceAreas.insert(utilityService->name(), utilityService );
                    ORIGIN_LOG_EVENT << "Started Utility Service area";

                    //
                    //	SDK 
                    //
                    SDK_ServiceArea* pSDK = SDK_ServiceArea::create(this, xmppService);
                    m_serviceAreas.insert(pSDK->name(), pSDK);	
                    ORIGIN_LOG_EVENT << "Started SDK Service area";

                    SDK_ECommerceServiceArea* pCommerceSDK = SDK_ECommerceServiceArea::create(this);
                    m_serviceAreas.insert( pCommerceSDK->name(), pCommerceSDK);	
                    ORIGIN_LOG_EVENT << "Started SDK Commerce Service area";

					ProgressiveInstallation_ServiceArea* pPISDK = ProgressiveInstallation_ServiceArea::create(this);
					m_serviceAreas.insert( pPISDK->name(), pPISDK);
					ORIGIN_LOG_EVENT << "Started SDK Progressive Installation Service area";

                    // Start service areas workers
                    for ( QHash<QString, BaseService*>::ConstIterator i = m_serviceAreas.begin(); i != m_serviceAreas.end(); ++i )
                    {
                        (*i)->startWorker();
                    }

                    ORIGIN_LOG_EVENT << "LSX workers started";

                    // Wait for all their asynchronous startup()s to finish
                    for ( QHash<QString, BaseService*>::ConstIterator i = m_serviceAreas.begin(); i != m_serviceAreas.end(); ++i )
                    {
                        (*i)->waitForAsyncStartup();
                    }

                    ORIGIN_LOG_EVENT << "All service areas have completed startup";

                }
            }
        }
    }
}
