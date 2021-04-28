#ifndef _SEARCHFRIENDOPERATIONPROXY_H
#define _SEARCHFRIENDOPERATIONPROXY_H

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
        
            class ORIGIN_PLUGIN_API SearchFriendsOperationProxy : public QObject
            {
                Q_OBJECT
            public:
                /// \brief CTOR
                /// \param parent The parent object.
                /// \param searchKeyword The search keyword.
                /// \param pageNumber The current page number.
                SearchFriendsOperationProxy(QObject *parent, const QString &searchKeyword, const QString& pageNumber = QString("1"));

                /// \brief executes the friend search
                Q_INVOKABLE void execute();

            signals:
                void failed();
                void succeeded(QVariantList);

            private slots:
                void searchSucceeded();
                void searchFailed();

            private:
                QString mSearchKeyword;
                QString mPageNumber;
            };
        }
    }
}


#endif