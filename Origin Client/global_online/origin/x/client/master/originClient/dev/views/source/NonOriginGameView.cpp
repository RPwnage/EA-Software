/////////////////////////////////////////////////////////////////////////////
// NonOriginGameView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "NonOriginGameView.h"
#include "ImageProcessingView.h"

#include "engine/content/LocalContent.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/login/LoginController.h"
#include "engine/subscription/SubscriptionManager.h"

#include "services/debug/DebugService.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/qt/QtUtil.h"

#include "TelemetryAPIDLL.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"
#include "originspinner.h"
#include "originlabel.h"
#include "originpushbutton.h"
#include "originbuttonbox.h"
#include "originwebview.h"
#include "ui_CloudDialog.h"
#include "ui_CustomizeBoxart.h"
#include "ui_RtpNotEntitledDialog.h"
#include "ui_ModifyNonOriginGameProperties.h"
#include "EbisuSystemTray.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>

#if defined(ORIGIN_PC)
#include <ShlObj.h>
#endif

namespace Origin
{
	namespace Client
	{

       // Design wanted to preview to be 141x200 - which is 61% of 231x326.
       const double NonOriginGameView::sPreviewScaling = 0.61;

        NonOriginGameView::NonOriginGameView(QWidget *parent) :
            QWidget(parent),
            mMessageBox(NULL)
        {
            // The message box itself is the only transient part
            // of the non-Origin game flow, so it's what needs to
            // be destroyed when the user logs out.
            ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onCanceled()));
        }

        NonOriginGameView::~NonOriginGameView()
        {

        }

        void NonOriginGameView::initialize()
        {

        }

        void NonOriginGameView::focus()
        {
            if(mMessageBox)
            {
                mMessageBox->raise();
                mMessageBox->activateWindow();
            }
        }

        UIToolkit::OriginWindow* NonOriginGameView::createConfirmRemoveDialog(const QString& gameName)
        {
            mMessageBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            mMessageBox->setTitleBarText(tr("application_name"));

            mMessageBox->msgBox()->setup( UIToolkit::OriginMsgBox::NoIcon, 
                tr("ebisu_client_confirm_remove_game_title"), tr("ebisu_client_confirm_remove_game_text").arg(gameName));
            mMessageBox->button(QDialogButtonBox::Ok)->setText(tr("ebisu_client_remove"));
            mMessageBox->button(QDialogButtonBox::Cancel)->setText(tr("ebisu_client_cancel"));
            mMessageBox->setDefaultButton(QDialogButtonBox::Cancel);

            return mMessageBox;
        }

        void NonOriginGameView::showConfirmRemoveDialog()
        {
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCanceled()));
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onAccepted()));
            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(onCanceled()));
            
            mMessageBox->setAttribute(Qt::WA_DeleteOnClose, false);
            mMessageBox->manager()->setupButtonFocus();

            mMessageBox->exec();
        }

        void NonOriginGameView::showGameAlreadyAddedDialog(const QString& gameName)
        {
            UIToolkit::OriginWindow* msgBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Close);
            msgBox->setTitleBarText(tr("ebisu_client_add_game_error_titlebar"));

            msgBox->msgBox()->setup( UIToolkit::OriginMsgBox::Error, 
                tr("ebisu_client_add_game_error_already_added").arg(gameName), "");
            msgBox->button(QDialogButtonBox::Close)->setText(tr("ebisu_client_close"));

            msgBox->manager()->setupButtonFocus();
            msgBox->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(msgBox, SIGNAL(rejected()), msgBox, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(msgBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), msgBox, SLOT(close()));

            msgBox->exec();
            msgBox->deleteLater();
        }

        void NonOriginGameView::showInvalidFileTypeDialog(const QString& fileName)
        {
            UIToolkit::OriginWindow* msgBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Close);
            msgBox->setTitleBarText(tr("ebisu_client_add_game_error_titlebar"));

            msgBox->msgBox()->setup( UIToolkit::OriginMsgBox::Error, 
                tr("ebisu_client_add_game_error_invalid_file").arg(fileName), "");
            msgBox->button(QDialogButtonBox::Close)->setText(tr("ebisu_client_close"));

            msgBox->manager()->setupButtonFocus();
            msgBox->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(msgBox, SIGNAL(rejected()), msgBox, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(msgBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), msgBox, SLOT(close()));

            msgBox->exec();
            msgBox->deleteLater();
        }

        void NonOriginGameView::showAddGameDialog()
        {
            mMessageBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::ScrollableMsgBox, QDialogButtonBox::Cancel);
            mMessageBox->setTitleBarText(tr("ebisu_client_add_game_titlebar"));
            // Will get delete by OriginWindow
            mMessageBox->scrollableMsgBox()->setup(UIToolkit::OriginScrollableMsgBox::NoIcon, tr("ebisu_client_add_game_title"), tr("ebisu_client_add_game_text"));
            
            Ui::RtpNotEntitledDialog ui;
            QWidget* commandlinkContent = new QWidget();
            ui.setupUi(commandlinkContent);

            // Hide button if the user isn't a subscriber
            if(Engine::Subscription::SubscriptionManager::instance()->isActive())
            {
                ui.link1->setText(tr("Subs_Library_AddGameMessage_Vault_Header"));
                ui.link1->setDescription(tr("Subs_Library_AddGameMessage_Vault_Body_Subscriber"));
                ui.link1->setEnabled(Services::Connection::ConnectionStatesService::isUserOnline(Engine::LoginController::instance()->currentUser()->getSession()));
                ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), ui.link1, SLOT(setEnabled(bool)));
                ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SLOT(onAccepted()));
                ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SIGNAL(showSubscriptionPage()));
            }
            else
            {
                ui.link1->hide();
            }

            ui.link2->setText(tr("ebisu_client_redeem_game_code_uppercase"));
            ui.link2->setDescription(tr("lockbox_code_entry"));
            ui.link2->setEnabled(Services::Connection::ConnectionStatesService::isUserOnline(Engine::LoginController::instance()->currentUser()->getSession()));
            ORIGIN_VERIFY_CONNECT(Origin::Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)), ui.link1, SLOT(setEnabled(bool)));
            ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SLOT(onAccepted()));
            ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SIGNAL(redeemGameCode()));

            ui.link3->setText(tr("ebisu_client_add_game_manual_title"));
            ui.link3->setDescription(tr("ebisu_client_add_game_manual_text").arg(tr("application_name")));
            ORIGIN_VERIFY_CONNECT(ui.link3, SIGNAL(clicked()), this, SLOT(onAccepted()));
            ORIGIN_VERIFY_CONNECT(ui.link3, SIGNAL(clicked()), this, SIGNAL(browseForGames()));

            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCanceled()));
            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(onCanceled()));
            
            mMessageBox->setAttribute(Qt::WA_DeleteOnClose, false);
            mMessageBox->manager()->setupButtonFocus();

            mMessageBox->setContent(commandlinkContent);
            mMessageBox->show();

            //normally we don't want to call activateWindow because it would steal focus if this window all of a sudden popped up while the user was on another application.
            //however since these windows are only triggered from menu or origin:// its ok. If we don't activate the window, when using origin:// the window will show up minimized
            mMessageBox->activateWindow();
        }

        void NonOriginGameView::onExecutableBrowseSelected()
        {

            QString currentExecutableLocation;
            if (QFile::exists(mProperties.getExecutableFile()))
            {
                QFileInfo fileInfo(mProperties.getExecutableFile());
                currentExecutableLocation = fileInfo.absolutePath();
            }
            else
            {
                #if defined(ORIGIN_PC)
                    WCHAR buffer[MAX_PATH];
                    SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, buffer );
                    currentExecutableLocation = QString::fromWCharArray(buffer);
                #elif defined(ORIGIN_MAC)
                    currentExecutableLocation = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::ApplicationsLocation);
                #else
                    #error Must specialize for other platform.
                #endif
            }

            QString filename = QFileDialog::getOpenFileName(EbisuSystemTray::instance()->primaryWindow(), "", currentExecutableLocation);
            if (!filename.isEmpty())
            {
                mProperties.setExecutableFile(filename);
                emit executableSelected(mProperties.getExecutableFile());
            }
        }

        void NonOriginGameView::onScanSelected()
        {
            onAccepted();
            emit scanForGames();
        }

        bool NonOriginGameView::showSelectGamesDialog()
        {
            return false;
        }

        void NonOriginGameView::showPropertiesDialog(Engine::Content::EntitlementRef eRef)
        {
            if (!eRef.isNull())
            {
                Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
                if (!c)
                    return;

                c->nonOriginController()->getProperties(Engine::Content::NonOriginContentController::getProductIdFromExecutable(
                    eRef->contentConfiguration()->executePath()), mProperties);

                mInitialDisplayName = mProperties.getDisplayName();

                using namespace UIToolkit;
                mMessageBox = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
                    NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                mMessageBox->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_game_properties_for_caps").arg(mProperties.getDisplayName().toUpper()), "");
            
                Ui::ModifyNonOriginGameProperties ui;
                QWidget* commandlinkContent = new QWidget();
                ui.setupUi(commandlinkContent);
#if defined(ORIGIN_MAC)
                ui.gridLayout->setHorizontalSpacing(14);
#endif
                ui.displayName->setText(mProperties.getDisplayName());
                ui.executableLocation->setText(mProperties.getExecutableFile());
                ui.exeParams->setText(mProperties.getExecutableParameters());
                ui.chkDisableOIG->setChecked(eRef->contentConfiguration()->igoPermission() == Origin::Services::Publishing::IGOPermissionBlacklisted);

                ORIGIN_VERIFY_CONNECT(ui.displayName, SIGNAL(textEdited(const QString&)), this, SLOT(onDisplayNameChanged(const QString&)));
                ORIGIN_VERIFY_CONNECT(ui.executableLocation, SIGNAL(textEdited(const QString&)), this, SLOT(onExecutableLocationChanged(const QString&)));
                ORIGIN_VERIFY_CONNECT(ui.exeParams, SIGNAL(textEdited(const QString&)), this, SLOT(onExecuteParametersChanged(const QString&)));
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(executableSelected(const QString&)), ui.executableLocation, SLOT(setText(const QString&)));
                ORIGIN_VERIFY_CONNECT(ui.btnBrowse, SIGNAL(clicked()), this, SLOT(onExecutableBrowseSelected()));
                ORIGIN_VERIFY_CONNECT(ui.chkDisableOIG, SIGNAL(clicked(bool)), this, SLOT(onOigManuallyDisabledChanged(const bool&)));

                mMessageBox->setContent(commandlinkContent);

                ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onPropertiesSet()));
                ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCanceled()));
                ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(onCanceled()));
            
                mMessageBox->setAttribute(Qt::WA_DeleteOnClose, false);
                mMessageBox->manager()->setupButtonFocus();

                mMessageBox->showWindow();
            }
        }

        void NonOriginGameView::onDisplayNameChanged(const QString& newName)
        {
            mProperties.setDisplayName(newName);
        }

        void NonOriginGameView::onExecutableLocationChanged(const QString& newLocation)
        {
            mProperties.setExecutableFile(newLocation);
        }

        void NonOriginGameView::onExecuteParametersChanged(const QString& newParams)
        {
            mProperties.setExecutableParameters(newParams);
        }

        void NonOriginGameView::onOigManuallyDisabledChanged(const bool& isChecked)
        {
            mProperties.setIgoEnabled(!isChecked);
        }

        void NonOriginGameView::onPropertiesSet()
        {
            // Strip unwanted characters from the title
            QString name = stripString(mProperties.getDisplayName());
            // If the title is not empty set it
            if (!name.isEmpty())
                mProperties.setDisplayName(name);
            // Can't allow the user to input an empty string as the game name
            else
            {
                QString title = stripString(mInitialDisplayName);
                if (title.isEmpty())
                {
                    // If our display name is still empty grab the file name
                    QString fileName = mProperties.getExecutableFile();
                    QFileInfo file(fileName);
                    QString originalTitle = file.baseName();
                    mProperties.setDisplayName(originalTitle);
                }
                else
                    mProperties.setDisplayName(mInitialDisplayName);
            }

            emit setProperties(mProperties, false);
            onAccepted();
        }

        void NonOriginGameView::showGameRepairDialog(const QString& gameName)
        {
            QString title(tr("ebisu_client_missing_game_files_title").arg(gameName));
            QString description = QString(tr("ebisu_client_missing_game_files_text")).arg(tr("application_name"));

            mMessageBox = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                NULL, UIToolkit::OriginWindow::MsgBox);

            mMessageBox->setTitleBarText(tr("ebisu_client_missing_game_files_titlebar"));
            mMessageBox->msgBox()->setup(UIToolkit::OriginMsgBox::NoIcon, title, description);
            
            Ui::CloudDialog ui;
            QWidget* commandlinkContent = new QWidget();
            ui.setupUi(commandlinkContent);

            ui.link1->setText(tr("ebisu_client_missing_game_files_locate_title"));
            ui.link1->setDescription(tr("ebisu_client_missing_game_files_locate_text").arg(tr("application_name")));
            ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SLOT(onAccepted()));
            ORIGIN_VERIFY_CONNECT(ui.link1, SIGNAL(clicked()), this, SIGNAL(browseForGames()));

            ui.link2->setText(tr("ebisu_client_missing_game_files_remove_title").arg(tr("application_name_uppercase")));
            ui.link2->setDescription(tr("ebisu_client_missing_game_files_remove_text"));
            ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SLOT(onAccepted()));
            ORIGIN_VERIFY_CONNECT(ui.link2, SIGNAL(clicked()), this, SIGNAL(removeSelected()));

            mMessageBox->addButton(QDialogButtonBox::Cancel);
            ORIGIN_VERIFY_CONNECT(mMessageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCanceled()));
            ORIGIN_VERIFY_CONNECT(mMessageBox, SIGNAL(rejected()), this, SLOT(onCanceled()));
            
            mMessageBox->setAttribute(Qt::WA_DeleteOnClose, false);
            mMessageBox->manager()->setupButtonFocus();

            mMessageBox->setContent(commandlinkContent);

            mMessageBox->exec();
        }

        void NonOriginGameView::onAccepted()
        {
            if (mMessageBox)
            {
                mMessageBox->accept();
                mMessageBox->deleteLater();
                mMessageBox = NULL;
            }
        }

        void NonOriginGameView::onCanceled()
        {
            if (mMessageBox)
            {
                ORIGIN_VERIFY_DISCONNECT(mMessageBox, SIGNAL(rejected()), 0, 0);
                mMessageBox->close();
                mMessageBox->deleteLater();
                mMessageBox = NULL;
            }

            emit cancel();
        }

        QString NonOriginGameView::stripString(const QString& string)
        {
            QString newString = string;
            // Need to remove these characters to prevent users from causing issues with social
            newString.remove(QRegExp("[\\x0000-\\x0008\\x000B\\x000C\\x000E-\\x001F]"));
            newString.remove(QRegExp("<[^>]*>"));
            return newString;
        }
	}
}
