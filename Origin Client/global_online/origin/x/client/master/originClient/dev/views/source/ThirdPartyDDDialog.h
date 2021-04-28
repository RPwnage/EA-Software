/////////////////////////////////////////////////////////////////////////////
// ThirdPartyDDDialog.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef ThirdPartyDDDialog_H
#define ThirdPartyDDDialog_H
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "services/publishing/CatalogDefinition.h"
#include "services/plugin/PluginAPI.h"

#include <QWidget>

namespace Ui
{
    class ThirdPartyDDDialog;
};

namespace Origin
{
    namespace Client
    {
        struct ThirdPartyDDParams
        {
            bool monitoredInstall;
            bool monitoredPlay;
            QString productId;
            Origin::Services::Publishing::PartnerPlatformType partnerPlatform;
            QString cdkey;
            QString displayName;
        };

        class ORIGIN_PLUGIN_API ThirdPartyDDWidget : public QWidget
        {
            Q_OBJECT
        public:
            enum ThirdPartyDDDialogType
            {
                ThirdPartyDD_InstallDialog,
                ThirdPartyDD_PlayDialog,
            };

            ThirdPartyDDWidget(ThirdPartyDDDialogType dialogType, const ThirdPartyDDParams& params, QWidget *parent = NULL);
            ~ThirdPartyDDWidget();

            void init();

            void setupExternalInstallDialog();
            void setupExternalPlayDialog();

            QString getPlatformName() { return mPlatformNameText;}

            public slots:
                void copyKeyToClipboard();
                bool copyKeyToClipViaHighlight();
                void linkActivated(const QString &);

        protected:
            void changeEvent(QEvent* event);
            bool eventFilter(QObject *obj, QEvent *event);
        private:
            void setPlatformName();
            void setText();

            Ui::ThirdPartyDDDialog* ui;
            ThirdPartyDDDialogType mDialogType;
            bool mIsStartDragInLicenseKey;
            QString mPlatformNameText;
            const ThirdPartyDDParams mParams;
        };
    }
}


#endif // ThirdPartyDDDialog_H
