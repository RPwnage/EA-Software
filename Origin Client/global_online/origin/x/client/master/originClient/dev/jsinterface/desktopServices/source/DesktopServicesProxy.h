#ifndef _DESKTOPSERVICESPROXY_H
#define _DESKTOPSERVICESPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/


#include <QObject>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            
            class DesktopServicesProxy : public QObject
            {
                Q_OBJECT
            public:
                DesktopServicesProxy(QObject *parent = NULL) { ; }
                ~DesktopServicesProxy() { ; }
                
                /// \brief Open the specified URL
                Q_INVOKABLE void asyncOpenUrl(const QString& url);
                Q_INVOKABLE void flashIcon(int unreadCount);
            private:
#ifdef ORIGIN_MAC
                void flashMac(int);
#endif
#ifdef ORIGIN_PC
                void flashPC();
#endif
            };
            
        }
    }
}


#endif //_DESKTOPSERVICESPROXY_H
