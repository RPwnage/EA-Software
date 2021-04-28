/////////////////////////////////////////////////////////////////////////////
// PDLCViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PDLCVIEWCONTROLLER_H
#define PDLCVIEWCONTROLLER_H

#include <QObject>

#include "engine/content/Entitlement.h"
#include "engine/igo/IGOController.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }
    namespace Client
    {
        class PDLCView;
        class ORIGIN_PLUGIN_API PDLCViewController : public QObject, public Origin::Engine::IPDLCStoreViewController, public Origin::Engine::IIGOWindowManager::IScreenListener
        {
            Q_OBJECT;

        public:
            PDLCViewController(QWidget* parent = NULL);
            ~PDLCViewController();

            // IPDLCStoreViewController impl
            virtual QWidget* ivcView() { return mPdlcWindow; }
            virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
            virtual void ivcPostSetup();

            virtual QUrl productUrl(const QString& productId, const QString& masterTitleId);
            virtual void show(const Origin::Engine::Content::EntitlementRef entitlement);
			virtual void purchase(const Origin::Engine::Content::EntitlementRef entitlement);
            virtual void showInIGO(const QString& url);
            virtual void showInIGOByCategoryIds(const QString& contentId, const QString& categoryIds, const QString& offerIds);
            virtual void showInIGOByMasterTitleId(const QString& contentId, const QString& masterTitleId, const QString& offerIds);

            // IScreenListener impl
            virtual void onSizeChanged(uint32_t width, uint32_t height);

            UIToolkit::OriginWindow* window() { return mPdlcWindow; };

        public slots:
            void setUrl(const QString &url);
            void close();
        
        signals:
            void cursorChanged(int cursorShape);

        private slots:
            void setCursor(int);
            /// \brief Called on the initial load finish of the PDLC page in OIG.
            /// We want to make sure the window doesn't overflow in the OIG.
            void onOIGInitialLoadFinish();
            void onViewSizeChanged(const QSize& size);

        private:
            void createPdlcView(bool isIgo);
            QPoint defaultPosition(uint32_t width = 0, uint32_t height = 0);
            
            QPointer<UIToolkit::OriginWindow> mPdlcWindow;
            QPointer<PDLCView> mPdlcView;

        };
    }
}
#endif