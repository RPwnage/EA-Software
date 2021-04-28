#ifndef _ACHIEVEMENTSHAREQUERYOPERATIONPROXY_H
#define _ACHIEVEMENTSHAREQUERYOPERATIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QVariant>
#include "AchievementShareOperationProxyBase.h"
#include "services/rest/HttpStatusCodes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
        
            class ORIGIN_PLUGIN_API AchievementShareQueryOperationProxy : public AchievementShareOperationProxyBase
            {
                Q_OBJECT
            public:
                AchievementShareQueryOperationProxy(QObject *parent);
            
            protected slots:
                /// \brief
                void onError(Origin::Services::HttpStatusCode);

            };
        }
    }
}


#endif