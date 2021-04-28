#include "QueryOperationProxyBase.h"

#include <QMetaObject>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            QueryOperationProxyBase::QueryOperationProxyBase(QObject *parent):
                QObject(parent)
            {
            }

            void QueryOperationProxyBase::failQuery()
            {
                emit failed();

                sender()->deleteLater();
                deleteLater();
            }

            void QueryOperationProxyBase::onQueryError(Origin::Services::HttpStatusCode sc)
            {
                int x = sc;
                emit queryError(x);
            }


        }
    }
}
