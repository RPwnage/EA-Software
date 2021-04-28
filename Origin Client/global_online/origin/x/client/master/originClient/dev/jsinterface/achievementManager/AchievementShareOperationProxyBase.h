#ifndef _ACHIEVEMENTSHAREOPERATIONPROXYBASE_H
#define _ACHIEVEMENTSHAREOPERATIONPROXYBASE_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QVariant>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {        
            class ORIGIN_PLUGIN_API AchievementShareOperationProxyBase : public QObject
            {
                Q_OBJECT
            public:
                AchievementShareOperationProxyBase(QObject *parent);
            
            signals:
                void failed();
                void succeeded(QVariant);
            
            protected slots:
                void onSuccess();
            
                void failQuery();

            };
        }
    }
}


#endif

