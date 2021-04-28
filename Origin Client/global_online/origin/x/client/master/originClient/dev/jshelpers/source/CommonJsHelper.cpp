/////////////////////////////////////////////////////////////////////////////
// CommonJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CommonJsHelper.h"
#include "version/version.h"
#include "services/platform/PlatformService.h"
#include "services/qt/QtUtil.h"
#include "engine/igo/IGOController.h"
#include "flows/MainFlow.h"
#include "TelemetryAPIDLL.h"

#include <QUrl>
#include <QWidget>
#include <QDesktopServices>
#include <QFileDialog>

namespace Origin
{
    namespace Client
    {
        CommonJsHelper::CommonJsHelper(QObject *parent) : QObject(parent)
        {
        }

        void CommonJsHelper::launchExternalBrowser(QString urlstring)
        {
            QUrl url(QUrl::fromEncoded(urlstring.toUtf8()));

            if (Engine::IGOController::instance()->isGameInForeground())
            {
                emit Engine::IGOController::instance()->showBrowser(urlstring, true);
            }
            else
            {
                Origin::Services::PlatformService::asyncOpenUrl(url);
            }
            GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
        }

#ifdef _WIN32
        void CommonJsHelper::launchInternetExplorer(QString urlstring)
        {
            QUrl url(urlstring);
            Origin::Services::PlatformService::asyncOpenUrl(url);
            GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
        }
#endif

        QString CommonJsHelper::selectFile(QString startingDirectory, QString filter, QString operation)
        {
            QString path, retval;

            if(startingDirectory.compare("desktop", Qt::CaseInsensitive) == 0)
            {
                path = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DesktopLocation);
            }
            else if(startingDirectory.compare("user_docs", Qt::CaseInsensitive) == 0)
            {
                path = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::DocumentsLocation);
            }
            else if(startingDirectory.compare("appdata", Qt::CaseInsensitive) == 0)
            {
                path = Origin::Services::PlatformService::commonAppDataPath();
            }

            if(operation.compare("save", Qt::CaseInsensitive) == 0)
            {
                retval = QFileDialog::getSaveFileName(NULL, QString(), path, filter);
            }
            else
            {
                retval = QFileDialog::getOpenFileName(NULL, QString(), path, filter);
            }

            return retval;
        }

        QString CommonJsHelper::ebisuVersion()
        {
            return QString(EBISU_VERSION_STR);
        }

        void CommonJsHelper::closePopUp()
        {
            if (parent()->isWidgetType())
            {
                QWidget* parentContainer = static_cast<QWidget*>( parent() );
                if( parentContainer ) 
                {
                    parentContainer->close();
                }
            }
        }

        void CommonJsHelper::doRetry()
        {
            emit(retry());
        }

        void CommonJsHelper::closeWindow()
        {
            emit closeWebWindow();
        }

    }
}

