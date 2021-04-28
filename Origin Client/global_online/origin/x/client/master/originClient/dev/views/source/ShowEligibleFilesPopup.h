///////////////////////////////////////////////////////////////////////////////
// ´ShowEligibleFilesPopup.h
// 
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef SHOWELIGIBLEFILESPOPUP_H
#define SHOWELIGIBLEFILESPOPUP_H


#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin {
    namespace UIToolkit {
        class OriginWindow;
    }
}

namespace Ui {
    class ShowEligibleFilesPopup;
}

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API ShowEligibleFilesPopup : public QObject
{
	Q_OBJECT

public:
    ShowEligibleFilesPopup(const QString& titleStr);
    ~ShowEligibleFilesPopup();

    void addFileList(const QStringList & filesList);
    void execDialog();
    void showDialog();
    void hideDialog();
    QStringList getFiles();

signals:
    void EnableOnClose();

protected:
    void init();

public slots:
    void closeEvent();
    void closeSettings();

private:
    QWidget* mListWidget;
    Ui::ShowEligibleFilesPopup* ui;
    Origin::UIToolkit::OriginWindow* mWindow;
    QString mTitleStr;


};
}
}

#endif

