/////////////////////////////////////////////////////////////////////////////
// InstallerView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef INSTALLERVIEW_H
#define INSTALLERVIEW_H

#include <QObject>
#include <QJsonArray>
#include "engine/downloader/ContentInstallFlowContainers.h"
#include "services/plugin/PluginAPI.h"
#include "eula.h"

namespace Origin
{
namespace Client
{
class ORIGIN_PLUGIN_API InstallerView : public QObject
{
    Q_OBJECT
public:
    InstallerView(QObject* parent = NULL);
    ~InstallerView();

    struct DownloadInfoContent{
        static const QString KEY_DESKTOPSHORTCUT;
        static const QString KEY_STARTMENUSHORTCUT;
        QString title;
        QString text;
        QJsonArray checkBoxContent;
        DownloadInfoContent(const Origin::Downloader::InstallArgumentRequest& request);
    };

    static QWidget* languageSelectionContent(const Origin::Downloader::InstallLanguageRequest& input, const QString& initLocale = "");
    static QJsonObject formatLanguageSelection(const Origin::Downloader::InstallLanguageRequest& input, const QString& initLocale = "");
    static QJsonObject checkObject(const QString& id, const QString& text, const QString& tooltip, const bool& checked);
    static QJsonObject eulaObject(Downloader::Eula& eula, const int& index, const int& count);
    static QString readTextFile(const QString& fileName);
    static QString convertRtfFileToHtml(const QString& fileName);
};
}
}
#endif