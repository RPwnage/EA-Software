#ifndef _QUERYOPERATIONPROXYBASE_H
#define _QUERYOPERATIONPROXYBASE_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include "services/rest/HttpStatusCodes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            class ORIGIN_PLUGIN_API QueryOperationProxyBase : public QObject
            {
                Q_OBJECT
            public:
                QueryOperationProxyBase(QObject *parent);

            signals:
                void failed();
                void queryError(int);

            protected slots:
                void failQuery();
                void onQueryError(Origin::Services::HttpStatusCode);


            };


        }
    }
}

#endif

