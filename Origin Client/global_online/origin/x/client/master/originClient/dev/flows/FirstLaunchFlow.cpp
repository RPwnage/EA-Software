///////////////////////////////////////////////////////////////////////////////
// FirstLaunchFlow.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "flows/FirstLaunchFlow.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"

#include "TelemetryAPIDLL.h"
#include "version/version.h"

#include "originmsgbox.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "origincheckbutton.h"
#include "originlabel.h"

#include "services/platform/PlatformService.h"
#include "services/platform/TrustedClock.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

namespace Origin
{
	namespace Client
	{
		FirstLaunchFlow::FirstLaunchFlow() :
            mAutoStart(true),
            mAutoUpdate(true),
            mSendAnonymousHWData(true)
		{
            // Check whether the machine locale is Canada -> auto-update is disabled initially (https://developer.origin.com/support/browse/ORPLAT-241)
            // Don't use OriginApplication and the like, as we do forward mappings to what Origin supports
            QString locale = QLocale::system().name();
            if (locale.endsWith("_CA", Qt::CaseInsensitive))
                mAutoUpdate = false;
		}

		void FirstLaunchFlow::start()
		{
            // This flow is currently only used on Mac, to allow the user to set 3 settings on the first
            // launch of the client, before the login window.  On PC, this is taken care of in the installer.
            // The 3 settings apply to all users; they are auto-update, auto-start, and share hardware data.
#ifdef ORIGIN_MAC
			bool isFirstLaunch = Origin::Services::readSetting(Origin::Services::SETTING_FIRSTLAUNCH);
            if(isFirstLaunch)
            {
                GetTelemetryInterface()->Metric_APP_INSTALL( EBISU_PRODUCT_VERSION_P_DELIMITED );
                
                showFirstTimeSettingsDialog();
                
                // SETTING_FirstLaunch is initially true.  After the First Launch flow runs, we set this
                // setting to false, so the First Launch flow never runs again.
                Origin::Services::writeSetting(Origin::Services::SETTING_FIRSTLAUNCH, false);

                // set install timestamp so we can determine if Keep Games Up To Date needs to be overridden by AUTOPATCHGLOBAL setting (ORPLAT-1034)
                qulonglong timestamp = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch() / 1000;
                Origin::Services::writeSetting(Origin::Services::SETTING_INSTALLTIMESTAMP, timestamp);
            }
#endif
		}

        void FirstLaunchFlow::showFirstTimeSettingsDialog()
        {
            using namespace Origin::UIToolkit;
            OriginWindow* window = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);

            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Ok), SIGNAL(clicked()), window, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(closeEventTriggered()), this, SLOT(onWindowClosed()));

            window->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_optional_settings_uppercase"), tr("ebisu_client_change_options"));
            window->manager()->setupButtonFocus();

            QWidget* windowContent = new QWidget();
            QGridLayout* grid = new QGridLayout(windowContent);
            // Add 10 to the spacing on Mac to match Windows UI
            grid->setContentsMargins(0, 0, 0, 0);
            grid->setColumnMinimumWidth(0, 25);
            grid->setSpacing(0);
            
            OriginCheckButton* ckAutoStart = new OriginCheckButton(windowContent);
            ckAutoStart->setChecked(mAutoStart);
            ckAutoStart->setText(tr("ebisu_client_open_at_login").arg(tr("application_name")));
            ORIGIN_VERIFY_CONNECT(ckAutoStart, SIGNAL(stateChanged(int)), this, SLOT(onAutoStartChanged(const int&)));
            grid->addWidget(ckAutoStart, 0, 0, 1, 2, Qt::AlignLeft);
            grid->setRowMinimumHeight(1, 13);
            
            //
            
            OriginCheckButton* ckAutoUpdateOrigin = new OriginCheckButton(windowContent);
            ckAutoUpdateOrigin->setChecked(mAutoUpdate);
            ckAutoUpdateOrigin->setText(tr("ebisu_client_auto_update_client_and_games"));
            ORIGIN_VERIFY_CONNECT(ckAutoUpdateOrigin, SIGNAL(stateChanged(int)), this, SLOT(onAutoupdateChanged(const int&)));
            grid->addWidget(ckAutoUpdateOrigin, 2, 0, 1, 2, Qt::AlignLeft);
            
            OriginLabel* autoUpdateExplainThis = new OriginLabel();
            autoUpdateExplainThis->setHyperlink(true);
            autoUpdateExplainThis->setWordWrap(false);
            autoUpdateExplainThis->setText(tr("ebisu_client_explain_this_link"));
            ORIGIN_VERIFY_CONNECT(autoUpdateExplainThis, SIGNAL(clicked()), this, SLOT(onAutoupdateExplainThisClicked()));
            grid->addWidget(autoUpdateExplainThis, 3, 1, 1, 1, Qt::AlignLeft);
            
            //
            
            OriginCheckButton* ckShareHardwareData = new OriginCheckButton(windowContent);
            ckShareHardwareData->setChecked(mSendAnonymousHWData);
            ckShareHardwareData->setText(tr("ebisu_client_send_hardware_data"));
            ORIGIN_VERIFY_CONNECT(ckShareHardwareData, SIGNAL(stateChanged(int)), this, SLOT(onSendAnonymousHWDataChanged(const int&)));
            grid->addWidget(ckShareHardwareData, 4, 0, 1, 2, Qt::AlignLeft);
            
            OriginLabel* shareHardwareDataExplainThis = new OriginLabel();
            shareHardwareDataExplainThis->setHyperlink(true);
            shareHardwareDataExplainThis->setWordWrap(false);
            shareHardwareDataExplainThis->setText(tr("ebisu_client_explain_this_link"));
            ORIGIN_VERIFY_CONNECT(shareHardwareDataExplainThis, SIGNAL(clicked()), this, SLOT(onSendAnonymousHWDataExplainThisClicked()));
            grid->addWidget(shareHardwareDataExplainThis, 5, 1, 1, 1, Qt::AlignLeft);

            //
            
            grid->setRowMinimumHeight(6, 26);
            
            OriginLabel* userWarning = new OriginLabel();
            userWarning->setWordWrap(true);
            userWarning->setText(tr("ebisu_client_confirm_user_read_explanations"));
            grid->addWidget(userWarning, 7, 0, 1, 2);
            
            window->setContent(windowContent);
            
#ifdef ORIGIN_MAC
            // Ensure the window will appear in Expose (i.e., Mission Control).
            Services::PlatformService::addWindowToExpose( window );
#endif
            window->execWindow();
        }

        void FirstLaunchFlow::onAutoStartChanged(const int& state)
        {
            mAutoStart = (state != 0);
        }

        void FirstLaunchFlow::onAutoupdateChanged(const int& state)
        {
            mAutoUpdate = (state != 0);
        }

        void FirstLaunchFlow::onAutoupdateExplainThisClicked()
        {
            UIToolkit::OriginWindow* window = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                                                      NULL, UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Ok);

            QString legalExplanation = tr("ebisu_client_automatic_update_legal_explanation");
            
            window->msgBox()->setup(UIToolkit::OriginMsgBox::NoIcon, "", legalExplanation, NULL);
            window->manager()->setupButtonFocus();
            
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Ok), SIGNAL(clicked()), window, SLOT(close()));
            
            window->execWindow();
        }
        
        void FirstLaunchFlow::onSendAnonymousHWDataChanged(const int& state)
        {
            mSendAnonymousHWData = (state != 0);
        }

        void FirstLaunchFlow::onSendAnonymousHWDataExplainThisClicked()
        {
            UIToolkit::OriginWindow* window = new UIToolkit::OriginWindow(UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close,
                                                                          NULL, UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Ok);
            
            QString faqSubstitution = tr("ebisu_client_faq");
            QString originSubstitution = tr("application_name");
            QString reportingText = tr("ebisu_client_experience_reporting_text").arg(originSubstitution).arg(faqSubstitution);
            
            window->msgBox()->setup(UIToolkit::OriginMsgBox::NoIcon, "", reportingText, NULL);
            window->manager()->setupButtonFocus();
            
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(rejected()), window, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Ok), SIGNAL(clicked()), window, SLOT(close()));
            
            window->execWindow();
        }

        void FirstLaunchFlow::onWindowClosed()
        {
            Origin::Services::writeSetting(Origin::Services::SETTING_RUNONSYSTEMSTART, mAutoStart);
            Origin::Services::writeSetting(Origin::Services::SETTING_AUTOUPDATE, mAutoUpdate);
            Origin::Services::writeSetting(Origin::Services::SETTING_AUTOPATCHGLOBAL, mAutoUpdate);
            Origin::Services::writeSetting(Origin::Services::SETTING_HW_SURVEY_OPTOUT, !mSendAnonymousHWData);
        }
        
	} // namespace Client
} // namespace Origin
