// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#ifndef CRASHREPORTWINDOW_H
#define CRASHREPORTWINDOW_H

#include <QDialog>

#include "CrashReportData.h"

#ifdef ORIGIN_MAC
#include "ui_crashreportwindow_mac.h"
#else
#include "ui_crashreportwindow.h"
#endif

namespace Ui {
class CrashReportWindow;
}

class CrashReportWindow : public QDialog
{
    Q_OBJECT
    
public:
    explicit CrashReportWindow( CrashReportData& crashData, QWidget *parent = 0 );
    ~CrashReportWindow();
    
signals:
    void signalReportShouldBeSent( bool );
    void dialogFinished( QString );
    void signalRestartOrigin();

private slots:
    void onClickLink();
    void onRestartClicked();
    void onExitClicked();

private:
#ifdef ORIGIN_MAC
    virtual void paintEvent(QPaintEvent* event);
#endif
    
    Ui::CrashReportWindow *ui;
    
    // The URL to open when clicking for crash report details. (i.e., the client log file)
    QString mLocalizedFaqUrl;
};

#endif // CRASHREPORTWINDOW_H
