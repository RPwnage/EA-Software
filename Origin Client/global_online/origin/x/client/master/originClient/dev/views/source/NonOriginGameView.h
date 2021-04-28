/////////////////////////////////////////////////////////////////////////////
// PlayView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NONORIGINGAMEVIEW_H
#define NONORIGINGAMEVIEW_H

#include "engine/content/Entitlement.h"
#include "engine/content/NonOriginContentController.h"
#include "services/plugin/PluginAPI.h"
#include "ui_CustomizeBoxart.h"

#include <QMovie>
#include <QWidget>

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }

    namespace Client
    {
        class ORIGIN_PLUGIN_API NonOriginGameView : public QWidget
        {
            Q_OBJECT

        public:

            static const double sPreviewScaling;

            NonOriginGameView(QWidget *parent = NULL);
            ~NonOriginGameView();

            void initialize();
            void focus();

            UIToolkit::OriginWindow* createConfirmRemoveDialog(const QString& gameName);

            void showAddGameDialog();
            bool showSelectGamesDialog();
            void showPropertiesDialog(Engine::Content::EntitlementRef eRef);
            void showGameRepairDialog(const QString& gameName);

            void showGameAlreadyAddedDialog(const QString& gameName);
            void showInvalidFileTypeDialog(const QString& fileName);
            void showConfirmRemoveDialog();
            QString stripString(const QString& string);

        signals:
            void browseForGames();
            void scanForGames();
            void redeemGameCode();
            void showSubscriptionPage();
            void cancel();
            void setProperties(const Origin::Engine::Content::NonOriginGameData& properties, bool forceUpdate);
            void executableSelected(const QString& filePath);
            void removeSelected();

        private slots:
            void onExecutableBrowseSelected();
            void onScanSelected();
            void onAccepted();
            void onCanceled();
            void onDisplayNameChanged(const QString& newName);
            void onExecutableLocationChanged(const QString& newLocation);
            void onExecuteParametersChanged(const QString& newParams);
            void onOigManuallyDisabledChanged(const bool& isChecked);
            void onPropertiesSet();

        private:

            UIToolkit::OriginWindow* mMessageBox;

            Origin::Engine::Content::NonOriginGameData mProperties;
            QString mInitialDisplayName;
        };
    }
}

#endif // NONORIGINGAMEVIEW_H
