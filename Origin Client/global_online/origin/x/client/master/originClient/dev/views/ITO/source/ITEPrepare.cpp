#include <QTimer>

#include "ITEPrepare.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "InstallDirectoryManager.h"
#include "engine/ito/InstallFlowManager.h"
#include "services/settings/SettingsManager.h"
#include "EbisuSystemTray.h"
#include "engine/content/Entitlement.h"
#include "engine/content/LocalContent.h"
#include "engine/content/ContentConfiguration.h"

#include "engine/content/ContentController.h"
#include "engine/downloader/ContentInstallFlowDIP.h"

#include "Qt/originbanner.h"
#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"
#include "Qt/origincheckbutton.h"
#include "Qt/origintooltip.h"
#include "ui_ITEPrepare.h"

#include "TelemetryAPIDLL.h"
#include "ITOFlow.h"
#include "services/debug/DebugService.h"

namespace Origin
{
    using namespace UIToolkit;

    namespace Client
    {
        void ITEPrepareWidget::setOptionalEulaList(const Downloader::EulaList& optionalEulaList)
        {
            for(int i = 0; i < optionalEulaList.listOfEulas.size(); i++)
            {
                ui->installSpaceInfo->addCheckButton(optionalEulaList.listOfEulas.at(i).InstallName, 
                    optionalEulaList.listOfEulas.at(i).ToolTip,
                    optionalEulaList.listOfEulas.at(i).CancelWarning);
            }
        }

        void ITEPrepareWidget::updateUI()
        {
            if((!Engine::Content::ContentController::currentUserContentController()->canChangeDipInstallLocation() && 
                Engine::Content::ContentController::currentUserContentController()->contentFolders()->downloadInPlaceFolderValid()))
            {
                ui->btnReset_dl_in_place_dir->setDisabled(true);
                ui->btnBrowseDownloadInPlaceLocation->setDisabled(true);
            }
            else
            {
                ui->btnReset_dl_in_place_dir->setDisabled(false);
                ui->btnBrowseDownloadInPlaceLocation->setDisabled(false);
            }
        }

        void ITEPrepareWidget::onDiskpaceRetry()
        {
            Engine::InstallFlowManager::GetInstance().SetStateCommand("Retry");
            updateDiskSpaceText();
        }

        // ---------------
        using namespace UIToolkit;

        ITEPrepareWidget::ITEPrepareWidget()
            :QWidget(0)
            ,mInstallDirectoryManager(NULL)
            ,mDiskspaceTimer(NULL)
        {
            ui = new Ui::ITEPrepare();
            ui->setupUi(this);
#if defined(ORIGIN_MAC)
            ui->horizontalLayout->setSpacing(14);
#endif
            init();
            ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                this, SLOT(updateData()));
            ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                this, SLOT(onDiskpaceRetry()));
        }

        ITEPrepareWidget::~ITEPrepareWidget()
        {
            if(mInstallDirectoryManager)
            {
                delete mInstallDirectoryManager;
                mInstallDirectoryManager = NULL;
            }


            if(mDiskspaceTimer)
            {
                delete mDiskspaceTimer;
                mDiskspaceTimer = NULL;
            }

        }

        InstallSpaceInfo* ITEPrepareWidget::installSpaceInfo() const
        {
            return ui->installSpaceInfo;
        }


        void ITEPrepareWidget::setInsufficientDiskState(bool insufficient)
        {
            Origin::UIToolkit::OriginWindow* window = dynamic_cast<Origin::UIToolkit::OriginWindow*>(topLevelWidget());
            if(window)
            {
                window->setBannerVisible(insufficient);
                if(window->button(QDialogButtonBox::Yes))
                    window->button(QDialogButtonBox::Yes)->setEnabled(!insufficient);
            }

            // Don't want to spam the logs with this so save our current state and only write it if it changes
            static bool currentDiskState = false;
            if (currentDiskState != insufficient)
            {
                ORIGIN_LOG_EVENT << "Install Through Ebisu: There is " << (insufficient ? "an insufficient" : "a sufficient") << " amount of disk space";
                currentDiskState = insufficient;

                if (insufficient)
                {
                    // Dump HD info to log
                    Services::PlatformService::PrintDriveInfoToLog();
                }
            }
        }

        void ITEPrepareWidget::retranslate()
        {
            ui->retranslateUi(this);
            updateData();
        }

        void ITEPrepareWidget::updateData()
        {
            ui->Inst_Location_Desc->setText(tr("ebisu_client_ite_installation_location_desc").arg(tr("application_name")));
            QString dipLocation = Services::readSetting(Services::SETTING_DOWNLOADINPLACEDIR, Services::Session::SessionService::currentSession());
            Engine::InstallFlowManager::GetInstance().SetVariable("InstallFolder", dipLocation);

            ui->downloadInPlaceLocation->setText( Engine::InstallFlowManager::GetInstance().GetVariable("InstallFolder").toString() );
            ui->downloadInPlaceLocation->cursorBackward(false, ui->downloadInPlaceLocation->maxLength() );

            updateDiskSpaceText();

            //check boxes
            ui->installSpaceInfo->desktopShortcut()->setChecked(Engine::InstallFlowManager::GetInstance().GetVariable("DesktopShortcut").toBool());
            ui->installSpaceInfo->startmenuShortcut()->setChecked(Engine::InstallFlowManager::GetInstance().GetVariable("ProgramShortcut").toBool());
        }

        void ITEPrepareWidget::updateDiskSpaceText()
        {
            //disc space
            uint64_t required_val = Engine::InstallFlowManager::GetInstance().GetVariable("InstallSize").toLongLong();
            ui->installSpaceInfo->setInstallSize(required_val);

            QString dipLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR, Origin::Services::Session::SessionRef());
            qulonglong available_val = Services::PlatformService::GetFreeDiskSpace(dipLocation);
            Engine::InstallFlowManager::GetInstance().SetVariable("AvailableSpace", available_val);
            ui->installSpaceInfo->setAvailableSpace(available_val);

            setInsufficientDiskState(ui->installSpaceInfo->sufficientFreeSpace() == false);
        }

        void ITEPrepareWidget::init()
        {
            mInstallDirectoryManager = new Client::JsInterface::InstallDirectoryManager();
            // Disconnecting the signals to the install managers. There is an InstallDirectoryManager already created from a singleton. If we don't
            // disconnect these signals then error messages will appear twice. This a band aid till we have a chance to move the 
            // functionality out of InstallDirectoryManager
            ORIGIN_VERIFY_DISCONNECT(Origin::Services::SettingsManager::instance(), 0, mInstallDirectoryManager, 0);
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Content::ContentController::currentUserContentController(), 0, mInstallDirectoryManager, 0);
            ORIGIN_VERIFY_DISCONNECT(Origin::Engine::Content::ContentController::currentUserContentController()->contentFolders(), 0, mInstallDirectoryManager, 0);
            mDiskspaceTimer = new QTimer(this);

            ui->btnBrowseDownloadInPlaceLocation->setAutoDefault(false);
            ui->btnReset_dl_in_place_dir->setAutoDefault(false);
            ORIGIN_VERIFY_CONNECT(ui->btnReset_dl_in_place_dir, SIGNAL(clicked()), mInstallDirectoryManager, SLOT(resetDownloadInPlaceLocation()));
            ORIGIN_VERIFY_CONNECT(ui->downloadInPlaceLocation, SIGNAL(textEdited(const QString &)), mInstallDirectoryManager, SLOT(resetDiPSignal()) );
            ORIGIN_VERIFY_CONNECT(ui->downloadInPlaceLocation, SIGNAL(editingFinished()), mInstallDirectoryManager, SLOT(dipChangeFinished()) );
            ORIGIN_VERIFY_CONNECT(ui->btnBrowseDownloadInPlaceLocation, SIGNAL(clicked()), this, SLOT(changeClicked()));
            ORIGIN_VERIFY_CONNECT(ui->installSpaceInfo->startmenuShortcut(), SIGNAL(stateChanged(int)), this, SLOT(onStartMenuShortcutStateChange(int)));
            ORIGIN_VERIFY_CONNECT(ui->installSpaceInfo->desktopShortcut(), SIGNAL(stateChanged(int)), this, SLOT(onDesktopShortcutStateChange(int)));
            ORIGIN_VERIFY_CONNECT(mDiskspaceTimer, SIGNAL(timeout()), this, SLOT(updateDiskSpaceText()));
    
            updateData();

            //check every 5 seconds for diskspace changes
            if(!mDiskspaceTimer->isActive())
            {
                mDiskspaceTimer->start(5000);
            }
        }
        void ITEPrepareWidget::changeClicked()
        {
            if(!Engine::Content::ContentController::currentUserContentController()->canChangeDipInstallLocation())
            {
                OriginWindow::alert(OriginMsgBox::Info, tr("ebisu_client_settings_uppercase"), tr("ebisu_client_settings_folder_change_error"), tr("ebisu_client_close"));
            }
            else
            {
                if( mInstallDirectoryManager )
                {
                    mInstallDirectoryManager->chooseDownloadInPlaceLocation(this);
                }
            }
        }

        void ITEPrepareWidget::onDesktopShortcutStateChange(int state)
        {
            Engine::InstallFlowManager::GetInstance().SetVariable("DesktopShortcut", state);
        }

        void ITEPrepareWidget::onStartMenuShortcutStateChange(int state)
        {
            Engine::InstallFlowManager::GetInstance().SetVariable("ProgramShortcut", state);
        }
    }
}

