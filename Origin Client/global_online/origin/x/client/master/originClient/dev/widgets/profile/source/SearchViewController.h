//    Copyright © 2012, Electronic Arts
//    All rights reserved.

#ifndef SEARCHVIEWCONTROLLER_H
#define SEARCHVIEWCONTROLLER_H

#include <QObject>
#include <QUrl>
#include <QSharedPointer>
#include "ViewControllerBase.h"
#include "engine/igo/IGOController.h"
#include "WebWidget/WidgetView.h"
#include "UIScope.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API SearchViewController : public ViewControllerBase, public Origin::Engine::ISearchViewController
        {
            Q_OBJECT
        public:
            SearchViewController(UIScope context, QWidget *parent);
            
            // ISearchViewController impl
            virtual QWidget* ivcView() { return view(); }
            virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
            virtual void ivcPostSetup() {}

            virtual bool isLoadSearchPage(const QString keyword);

        private:
            WebWidget::WidgetLink widgetLinkForSearch(const QString& keyword);
        };

    }
} 

#endif //MYGAMESVIEWCONTROLLER_H
