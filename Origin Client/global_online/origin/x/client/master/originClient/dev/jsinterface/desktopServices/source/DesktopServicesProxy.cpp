#include "DesktopServicesProxy.h"
#include "PopOutController.h"
#include "ClientFlow.h"
#include "services/platform/PlatformService.h"
#include "services/log/LogService.h"

#include <QApplication>
#include <QWindow>

#ifdef ORIGIN_MAC
#include "OriginApplicationDelegateOSX.h"
#endif

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            void DesktopServicesProxy::asyncOpenUrl(const QString& url)
            {
                Origin::Services::PlatformService::asyncOpenUrl(QUrl(url));
            }

            void DesktopServicesProxy::flashIcon(int unreadCount)
            {
                ORIGIN_LOG_EVENT << "We recieved an unread message and need to flash the icon.";
#ifdef ORIGIN_MAC
                flashMac(unreadCount);
#endif
#ifdef ORIGIN_PC
                flashPC();
#endif
            }
            
#ifdef ORIGIN_MAC
            void DesktopServicesProxy::flashMac(int unreadCount)
            {
                Origin::Client::setDockTile(unreadCount);
                // Check for a popped out Chat window
                UIToolkit::OriginWindow* chatWindow = Origin::Client::PopOutController::instance()->chatWindow();
                if (chatWindow)
                {
                    QApplication::alert(ClientFlow::instance()->view()->window()->topLevelWidget(), 1500);
                }
                else // No popped out chat window do something with the main SPA
                {
                    ClientFlow::instance()->view()->window();
                    QApplication::alert(ClientFlow::instance()->view()->window()->topLevelWidget(), 1500);
                }
            
            }
#endif
#ifdef ORIGIN_PC
            void DesktopServicesProxy::flashPC()
            {
                // Check for a popped out Chat window
                UIToolkit::OriginWindow* chatWindow = Origin::Client::PopOutController::instance()->chatWindow();
                if (chatWindow)
                {
                    Services::PlatformService::flashTaskbarIcon(chatWindow->winId());
                }
                else // No popped out chat window do something with the main SPA
                {
                    ClientFlow::instance()->view()->window();
                    Services::PlatformService::flashTaskbarIcon(ClientFlow::instance()->view()->window()->winId());
                }
            }
#endif

        }
    }
}

