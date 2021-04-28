//    Copyright © 2013, Electronic Arts
//    All rights reserved.

#ifndef VIEWCONTROLLERBASE_H
#define VIEWCONTROLLERBASE_H

#include <QObject>
#include <QUrl>
#include <QSharedPointer>

#include "../../navigation/Source/NavigationController.h"
#include "../../../originClient/dev/common/source/UIScope.h"

#include "services/plugin/PluginAPI.h"


namespace WebWidget
{
    class WidgetView;
    class WidgetLink;
}

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API ViewControllerBase : public QObject
        {
            Q_OBJECT
        public:
            ViewControllerBase(UIScope context, NavigationItem naviItem, QWidget *parent);
            virtual ~ViewControllerBase();

            WebWidget::WidgetView* view() const;

            /// \brief Called from the Javascript to know what our context is.
            /// If we are in OIG some elements will be hidden or disabled.
            Q_INVOKABLE int context() const {return mContext;}

        protected slots:
            void onUrlChanged(const QUrl& url);

        protected:

            WebWidget::WidgetView*  mView;

            NavigationItem mNaviItem;

            const UIScope mContext;

        private:
            ViewControllerBase(const ViewControllerBase&);
            ViewControllerBase& operator=(ViewControllerBase&);
        };

    }
} 

#endif //MYGAMESVIEWCONTROLLER_H
