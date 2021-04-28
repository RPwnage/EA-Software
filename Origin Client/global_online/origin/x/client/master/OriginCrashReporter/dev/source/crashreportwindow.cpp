// Copyright (c) Electronic Arts, Inc. -- All Rights Reserved.

#include "crashreportwindow.h"
#ifdef ORIGIN_MAC
#include "ui_crashreportwindow_mac.h"
#include "MacNativeSupport.h"
#else
#include "ui_crashreportwindow.h"
#endif
#include <QDesktopServices>
#include <QUrl>


CrashReportWindow::CrashReportWindow( CrashReportData& crashData, QWidget *parent )
    : QDialog(parent)
    , ui(new Ui::CrashReportWindow)
    , mLocalizedFaqUrl( crashData.mLocalizedFaqUrl )
{
#ifdef ORIGIN_MAC
    // EBIBUGS-20589: Hides all window titlebar controls such as close, minimize, maximize, zoom buttons.
    hideTitlebarDecorations(this);
#else
    // EBIBUGS-20589: Hides all window titlebar controls such as close, minimize, maximize, zoom buttons.
    setWindowFlags( Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint );
#endif

    ui->setupUi(this);
    
    // Ensure the app quits when this window closes.
    setAttribute( Qt::WA_QuitOnClose );

    connect(ui->linkLabel, SIGNAL(clicked()), this, SLOT(onClickLink()));
    connect(ui->checkBox, SIGNAL(clicked(bool)), this, SIGNAL(signalReportShouldBeSent(bool)));
    connect(ui->restartButton, SIGNAL(clicked()), this, SLOT(onRestartClicked()));
    connect(ui->exitButton, SIGNAL(clicked()), this, SLOT(onExitClicked()));

    QString linkString(QString("<html><p><a href=\"%1\">%2</a></p></html>").arg(tr("origin.com")).arg(tr("ebisu_client_crash_reporter_link")));

#ifdef ORIGIN_PC
    this->setWindowTitle(tr("ebisu_client_crash_reporter_title").arg(tr("application_name")));
#else
    // Disable Window Title.  Ref EBIBUGS-20589
    this->setWindowTitle("");
#endif

    ui->title->setText(tr("ebisu_client_crash_reporter_header").arg(tr("application_name")));
    ui->message->setText(tr("ebisu_client_crash_reporter_text").arg(tr("application_name")));
    ui->linkLabel->setText(linkString);
    ui->checkBox->setText(tr("ebisu_client_crash_reporter_checkbox"));
    ui->additionalInfoLabel->setText(tr("ebisu_client_crash_reporter_additional_info_label"));
#ifdef ORIGIN_MAC
    ui->restartButton->setText(tr("ebisu_client_relaunch_EADM_btn").arg(tr("application_name")));   // Relaunch %1
    ui->exitButton->setText(tr("ebisu_mainmenuitem_quit_app").arg(tr("application_name")));         // Quit %1
#else
    ui->restartButton->setText(tr("ebisu_client_restart_application").arg(tr("application_name")));
    ui->exitButton->setText(tr("ebisu_client_exit_app").arg(tr("application_name")));
#endif
}

CrashReportWindow::~CrashReportWindow()
{
    delete ui;
}

#ifdef ORIGIN_MAC
void CrashReportWindow::paintEvent(QPaintEvent* event)
{
    // For debugging layout only!
    //setOpacity(this, 0.9f);
    
    static bool forcedFocus = false;
    if (!forcedFocus)
    {
        // Make sure to give the window focus
        forceFocusOnWindow(this);
        forcedFocus = true;
    }
    
    QDialog::paintEvent(event);
}
#endif

void CrashReportWindow::onClickLink()
{
    QDesktopServices::openUrl( QUrl( mLocalizedFaqUrl ) );
}

void CrashReportWindow::onRestartClicked()
{
    hide();
    emit dialogFinished( ui->additionalInfo->toPlainText() );
    emit signalRestartOrigin();
    accept();
}

void CrashReportWindow::onExitClicked()
{
    hide();
    emit dialogFinished( ui->additionalInfo->toPlainText() );
    reject();
}

