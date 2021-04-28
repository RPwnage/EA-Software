/////////////////////////////////////////////////////////////////////////////
// ThirdPartyDDDialog.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ThirdPartyDDDialog.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/qt/QtUtil.h"

#include <QMouseEvent>
#include <QClipboard>
#include <QTextCursor>
// TODO: FIXME PLEASE! This is a private header in Qt.
#include <qwidgettextcontrol_p.h>
#include "TelemetryAPIDLL.h"
#include "engine/login//LoginController.h"
#include "services/debug//DebugService.h"
#include "engine/downloader/IContentInstallFlow.h"
#include "ui_ThirdPartyDDDialog.h"
#include "originwindow.h"
#include "originscrollablemsgbox.h"
#include "originwindowmanager.h"
#include "origincheckbutton.h"

// removed thirdPartyDDTitle & summary those are title and text


using namespace Origin::Engine;
namespace Origin
{
    namespace Client
    {
ThirdPartyDDWidget::ThirdPartyDDWidget(ThirdPartyDDDialogType dialogType, const ThirdPartyDDParams& params, QWidget *parent)
    : QWidget()
    , mDialogType(dialogType)
    ,mIsStartDragInLicenseKey(false)
    ,mParams(params)

{
    ui = new Ui::ThirdPartyDDDialog();
    ui->setupUi(this);
    init();
}

ThirdPartyDDWidget::~ThirdPartyDDWidget()
{
    delete ui;
}
void ThirdPartyDDWidget::init()
{
    switch(mDialogType)
    {
    case ThirdPartyDD_InstallDialog:
        setupExternalInstallDialog();
        break;
    case ThirdPartyDD_PlayDialog:
        setupExternalPlayDialog();
        break;
    default:
        break;
    }

    ORIGIN_VERIFY_CONNECT(ui->step2summary, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));


    //Add event filter to deal with the key code copy event
    ui->cdKeyValue->installEventFilter(this);

}

void ThirdPartyDDWidget::setPlatformName()
{
    if(mParams.partnerPlatform == Origin::Services::Publishing::PartnerPlatformSteam)
        mPlatformNameText = tr("ebisu_client_steam");
    else if(mParams.partnerPlatform == Origin::Services::Publishing::PartnerPlatformGamesForWindows)
        mPlatformNameText = tr("ebisu_client_games_for_windows_live");
    else
        mPlatformNameText = tr("ebisu_client_3PDD_an_external_application");
}

QString getPartnerPlatformTelemetryIdentifier(Origin::Services::Publishing::PartnerPlatformType platform)
{
    QString gameType = "origin";

    switch(platform)
    {
    case Origin::Services::Publishing::PartnerPlatformSteam:
        gameType = "steam";
        break;
    case Origin::Services::Publishing::PartnerPlatformGamesForWindows:
        gameType = "gfwl";
        break;
    case Origin::Services::Publishing::PartnerPlatformOtherExternal:
        gameType = "other";
        break;
    default:
        // explicitly ignore other cases to keep clang quiet
        break;
    }

    return gameType;
}

void ThirdPartyDDWidget::setupExternalInstallDialog()
{
    GetTelemetryInterface()->Metric_3PDD_INSTALL_DIALOG_SHOW();
    GetTelemetryInterface()->Metric_3PDD_INSTALL_TYPE(getPartnerPlatformTelemetryIdentifier(mParams.partnerPlatform).toUtf8().data());

    setText();
    ORIGIN_VERIFY_CONNECT(ui->copyToClipBtn, SIGNAL(clicked()),this, SLOT(copyKeyToClipboard()));
}

    void ThirdPartyDDWidget::setupExternalPlayDialog()
    {
        GetTelemetryInterface()->Metric_3PDD_PLAY_DIALOG_SHOW();
        GetTelemetryInterface()->Metric_3PDD_PLAY_TYPE(getPartnerPlatformTelemetryIdentifier(mParams.partnerPlatform).toUtf8().data());

        setText();

        ORIGIN_VERIFY_CONNECT(ui->copyToClipBtn, SIGNAL(clicked()),this, SLOT(copyKeyToClipboard()));
    }

    void ThirdPartyDDWidget::copyKeyToClipboard()
    {
        if(mDialogType == ThirdPartyDD_InstallDialog)
            GetTelemetryInterface()->Metric_3PDD_INSTALL_COPYCDKEY();
        else
            GetTelemetryInterface()->Metric_3PDD_PLAY_COPYCDKEY();
        
        QApplication::clipboard()->setText(ui->cdKeyValue->text());
        
    }
    void ThirdPartyDDWidget::linkActivated(const QString &link)
    {
        Origin::Services::PlatformService::asyncOpenUrl(QUrl(link));
    }

    //called when we change the language while the dialog is up
    void ThirdPartyDDWidget::setText()
    {
        setPlatformName();
        switch(mDialogType)
        {
        case ThirdPartyDD_InstallDialog:
            {
                const QString steamHelpURL = Services::readSetting(Services::SETTING_SteamSupportURL);

                //step1 text
                ui->step1->setText(tr("ebisu_client_3PDD_1_copy_your_product_code"));
                ui->step1summary->setText(tr("ebisu_client_3PDD_copy_your_product_code_desc"));

                //step 2 text
                ui->step2->setText(tr("ebisu_client_3PDD_2_Install_Through").arg(mPlatformNameText));
                if(mParams.monitoredInstall)
                    ui->step2summary->setText(tr("ebisu_client_3PDD_monitored_install_through_desc").arg(mParams.displayName).arg(mPlatformNameText).arg(mPlatformNameText).arg(tr("application_name")));
                else
                {
                    QString steamInstallText = tr("ebisu_client_3PDD_install_through_desc").arg(mParams.displayName).arg(mPlatformNameText).arg(tr("application_name"));

                    if(mParams.partnerPlatform == Origin::Services::Publishing::PartnerPlatformSteam)
                    {
                        steamInstallText += " ";
                        steamInstallText += tr("ebisu_client_steam_support").arg(steamHelpURL);
                    }

                    ui->step2summary->setText(steamInstallText);
                }
                //code copy
                ui->cdKeyLabel->setText(tr("ebisu_client_product_code_with_colon"));
                ui->cdKeyValue->setText(mParams.cdkey);
                ui->copyToClipBtn->setText(tr("ebisu_client_copy_cd_key_to_clip"));
                ui->copyToClipBtn->setFocus();

            }
            break;
        case ThirdPartyDD_PlayDialog:
            {
                ui->cdKeyLabel->setText(tr("ebisu_client_product_code_with_colon"));
                ui->cdKeyValue->setText(mParams.cdkey);
                
                ui->copyToClipBtn->setText(tr("ebisu_client_copy_cd_key_to_clip"));
                ui->copyToClipBtn->setFocus();

                //hide the frames
                ui->step1frame->hide();
                ui->step2frame->hide();
            }
            break;
        default:
            break;
        }

//        this->adjustSize();
    }

    void ThirdPartyDDWidget::changeEvent( QEvent* event )
    {
        if ( event->type() == QEvent::LanguageChange )
        {
            setText();
        }

        QWidget::changeEvent( event );
    }

    bool ThirdPartyDDWidget::eventFilter(QObject *obj, QEvent *event)
    {
#ifdef WIN32
        // filter label's ctrl+c event
        if(obj == ui->cdKeyValue 
            && event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if(keyEvent->matches(QKeySequence::Copy))
            {
                bool bCopySuccess = this->copyKeyToClipViaHighlight();

                // eventFilter will return if and only if we can copy with our own method
                if(bCopySuccess)
                {
                    return true;
                }
                // else let system do by itself
            }
        }
        // filter label's contextMenu event
        else if(obj == ui->cdKeyValue
            && event->type() == QEvent::ContextMenu)
        {
            QContextMenuEvent *contextMenuEvent = static_cast<QContextMenuEvent *>(event);
            QWidgetTextControl* control = ui->cdKeyValue->findChild<QWidgetTextControl*>();
            QMenu* menu = control->createStandardContextMenu(contextMenuEvent->pos(), ui->cdKeyValue);

            if(menu)
            {
                QList<QAction *> actionList = menu->actions();
                for(QList<QAction *>::iterator iter = actionList.begin(); iter!=actionList.end(); ++iter)
                {
                    QString textS((*iter)->text());

                    // find this period code in the source code of the QObject who have createStandardContextMenu method
#pragma message("Qt5 QT_NO_SHORTCUT was not defined in Qt4 - ThirdPartyDDWidget::eventFilter")
#ifndef QT_NO_SHORTCUT
                    QString strAccelCopy = QLatin1Char('\t') + QString(QKeySequence::Copy);
#else
                    QString strAccelCopy = QString();
#endif
                    if (textS.compare(tr("ebisu_client_menu_copy") + strAccelCopy) == 0 )
                    {
                        ORIGIN_VERIFY_CONNECT((*iter), SIGNAL(triggered()), this, SLOT(copyKeyToClipViaHighlight()));
                    }
                }
                // round corner
                Services::DrawRoundCornerForMenu(menu, 6);
                menu->exec(contextMenuEvent->globalPos());
                delete menu;
                // eventFilter will return if and only if we create menu successfully
                return true;
            }
        }
#endif
        // add mouse drag and highlight feature for this label
        if(obj == ui->cdKeyValue)
        {
            // get the licensekey label's textcontrol
            QWidgetTextControl* control = ui->cdKeyValue->findChild<QWidgetTextControl*>();
            if(control)
            {
                // get the label's textcursor
                QTextCursor cursor = control->textCursor();
                switch(event->type())
                {
                case QEvent::MouseButtonPress:
                    {
                        QMouseEvent *mousePressEvent = static_cast<QMouseEvent *>(event);
                        if(mousePressEvent->button() == Qt::LeftButton)
                        {
                            // clear the selection when we click the left mouse button
                            cursor.clearSelection();
                            // reset the textcursor
                            control->setTextCursor(cursor);
                            mIsStartDragInLicenseKey = true;
                        }
                        break;
                    }
                case QEvent::MouseMove:
                    {
                        QMouseEvent *mousePressEvent = static_cast<QMouseEvent *>(event);

                        if(mIsStartDragInLicenseKey)
                        {
                            // translate the cursor position to current text cursor position 
                            int targetPosition = control->cursorForPosition(mousePressEvent->pos()).position();
                            // reset the textcursor position
                            cursor.setPosition(targetPosition, QTextCursor::KeepAnchor);
                            control->setTextCursor(cursor);					
                        }
                    }
                    break;
                case QEvent::MouseButtonRelease:
                    {
                        // when mouse release, reset the drag flag
                        QMouseEvent *mousePressEvent = static_cast<QMouseEvent *>(event);
                        if(mousePressEvent->button() == Qt::LeftButton)
                        {
                            mIsStartDragInLicenseKey = false;
                        }
                    }
                    break;
                default:
                    break;
                }
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    bool ThirdPartyDDWidget::copyKeyToClipViaHighlight()
    {
        QWidgetTextControl* control = ui->cdKeyValue->findChild<QWidgetTextControl*>();
        if(control)
        {
            // call default copy method first
            // For the encapsulation may do something we don't metion
            control->copy();
        }

        return true;
    }
}
}
