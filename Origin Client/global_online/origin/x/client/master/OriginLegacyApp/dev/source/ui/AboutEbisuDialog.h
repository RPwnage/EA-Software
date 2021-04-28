///////////////////////////////////////////////////////////////////////////////
// AboutEbisuDialog.h
// 
// Copyright (c) 2010-2012, Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef ABOUTEBISUDIALOG_H
#define ABOUTEBISUDIALOG_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Origin 
{
namespace UIToolkit
{
    class OriginWindow;
    class OriginMsgBox;
}
namespace Client
{
    class OriginWebController;
}
}

namespace Ui
{
    class AboutEbisuDialog;
}
class QWebView;

class ORIGIN_PLUGIN_API AboutEbisuDialog : public QWidget 
{
	Q_OBJECT
public:
	AboutEbisuDialog(QWidget *parent = NULL);
    ~AboutEbisuDialog();

protected:
	void changeEvent( QEvent* event );

public slots:
    void linkActivated(const QString &);
	void onReleaseNotesLoaded();
    void showReleaseNotes();
    void closeReleaseNotes();

private:
	void retranslate();
	Ui::AboutEbisuDialog* ui;
    Origin::UIToolkit::OriginWindow* mDialogChrome;
	Origin::UIToolkit::OriginMsgBox* mBoxChrome;
    Origin::Client::OriginWebController* mWebController;
    QWebView* mWebView;
};

#endif
