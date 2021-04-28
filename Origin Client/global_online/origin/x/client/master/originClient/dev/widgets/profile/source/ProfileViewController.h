#ifndef _PROFILEVIEWCONTROLLER_H
#define _PROFILEVIEWCONTROLLER_H

#include <QObject>
#include <QWebHistory>

#include "UIScope.h"
#include "ViewControllerBase.h"
#include "engine/igo/IGOController.h"
#include "WebWidget/WidgetView.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API ProfileViewController : public ViewControllerBase, public Origin::Engine::IProfileViewController
        {
            Q_OBJECT
            
        public:
            ProfileViewController(UIScope context, const quint64& initialNucleusId, QWidget* parent);
            
            bool showProfileForNucleusId(quint64 nucleusId);
            virtual QWidget* ivcView() { return view(); }
            virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior) { return true; }
            virtual void ivcPostSetup() {}

            virtual void resetNucleusId();
            virtual quint64 nucleusId() const;

            /// \brief retuns the start URL for the profile
            QUrl startUrl() const;

            /// \brief retuns the history for the profile
            QWebHistory* history() const;

            static quint64 nucleusIdByUserId(const QString& id);

            public slots:
                /// \brief Let's the JavaScript access this class. Used for telling the the 
                /// web page what context we are in
                void populatePageJavaScriptWindowObject();

        private:
            WebWidget::WidgetLink widgetLinkForNucleusId(quint64 nucleusId);

            quint64 mNucleusId;
        };
    }
} 

#endif

