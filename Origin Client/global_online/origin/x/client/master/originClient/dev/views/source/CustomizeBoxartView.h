/////////////////////////////////////////////////////////////////////////////
// CustomizeBoxartView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CUSTOMIZEBOXARTVIEW_H
#define CUSTOMIZEBOXARTVIEW_H

#include "engine/content/Entitlement.h"
#include "services/entitlements/BoxartData.h"
#include "services/plugin/PluginAPI.h"
#include "ui_CustomizeBoxart.h"

#include <QMovie>
#include <QWebView>
#include <QWidget>

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
    }

    namespace Client
    {
        class ORIGIN_PLUGIN_API CustomizeBoxartView : public QWidget
        {
            Q_OBJECT

        public:

            static const double sPreviewScaling;

            CustomizeBoxartView(QWidget *parent = NULL);
            ~CustomizeBoxartView();

            void initialize();
            void focus();
            void showCustomizeBoxartDialog(Engine::Content::EntitlementRef eRef);
            void showInvalidFileTypeDialog(const QString& fileName);

        signals:

            void cancel();
            void setBoxart(const Services::Entitlements::Parser::BoxartData& boxartData);
            void removeBoxart();
            void boxartSelected(const QString& filePath);
            void enableRestoreButton(bool);
            void enableCropButton(bool);
            void updateBoxart(const QPixmap& pixmap);
            void updateBoxart(QMovie* movie);

        private slots:
            void onBoxartBrowseSelected();
            void onCropImageSelected();
            void onCropBoxart();
            void onAccepted();
            void onCanceled();
            void onRestoreDefaultBoxart();
            void onBoxartSet();
            bool onUpdateBoxartPreview(const QString& filename);
            void onUpdateBoxartPreview(const QPixmap& pixmap);
            void onUpdateBoxartPreview(const QUrl& url);

            void sendCustomizeBoxartApplyTelemetry();
            void sendCustomizeBoxartCancelTelemetry();

            void onLoadDefaultBoxartUrlComplete(QNetworkReply* reply);
            void onCropImageLoaded();

        private:

            UIToolkit::OriginWindow* mMessageBox;

            Services::Entitlements::Parser::BoxartData mBoxartData;

            QString mCurrentBoxartFile;
			QString mCurrentBoxartBaseFile;
            QPixmap mDefaultBoxart;
            bool mIsNonOriginGame;
            QUrl mBoxartDefaultUrl;

            QWebView* mPreviewWebView;

            QWidget* initializeCustomizeBoxartDialog(Ui::CustomizeBoxart& ui);
            bool isValidImageFile(const QString& imageFile);

            QString previewHtml(const QString& imageUrl, int imageWidth, int imageHeight);
        };
    }
}

#endif // CUSTOMIZEBOXARTVIEW_H
