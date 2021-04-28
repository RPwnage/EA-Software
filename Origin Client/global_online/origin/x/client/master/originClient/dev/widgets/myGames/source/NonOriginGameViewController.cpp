
//    Copyright © 2012 Electronic Arts
//    All rights reserved.

#include "NonOriginGameViewController.h"
#include "flows/NonOriginGameFlow.h"

#include "engine/content/ContentConfiguration.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"

#include "originpushbutton.h"
#include "originwindow.h"

#include <QFileDialog>
#include "EbisuSystemTray.h"

#if defined(ORIGIN_PC)
#include <ShlObj.h>
#endif

namespace Origin
{
    namespace Client
    {
        NonOriginGameViewController::NonOriginGameViewController(NonOriginGameFlow* parent)
            : QObject(parent)
            , mParent(parent)
            , mNonOriginGameView(NULL)
            , mShowingRepairDialog(false)
        {
 
        }

        NonOriginGameViewController::~NonOriginGameViewController()
        {

        }
            
        void NonOriginGameViewController::initialize()
        {
            mNonOriginGameView = new NonOriginGameView();
            mNonOriginGameView->initialize();

            ORIGIN_VERIFY_CONNECT_EX(mNonOriginGameView, SIGNAL(browseForGames()), this, SLOT(onBrowseForGames()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mNonOriginGameView, SIGNAL(redeemGameCode()), this, SIGNAL(redeemGameCode()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT_EX(mNonOriginGameView, SIGNAL(showSubscriptionPage()), this, SIGNAL(showSubscriptionPage()), Qt::QueuedConnection);
            ORIGIN_VERIFY_CONNECT(mNonOriginGameView, SIGNAL(setProperties(const Origin::Engine::Content::NonOriginGameData&, bool)), this,
                SIGNAL(propertiesChanged(const Origin::Engine::Content::NonOriginGameData&, bool)));
            ORIGIN_VERIFY_CONNECT_EX(mNonOriginGameView, SIGNAL(removeSelected()), this, SIGNAL(removeBrokenGame()), Qt::QueuedConnection);

            ORIGIN_VERIFY_CONNECT_EX(mNonOriginGameView, SIGNAL(cancel()), mParent, SLOT(cancel()), Qt::QueuedConnection);
        }

        void NonOriginGameViewController::focus()
        {
            mNonOriginGameView->focus();
        }

        void NonOriginGameViewController::onShowAddGameDialog()
        {
            mNonOriginGameView->showAddGameDialog();
        }

        void NonOriginGameViewController::onShowSelectGamesDialog(const QList<Engine::Content::ScannedContent>& games)
        {
            mNonOriginGameView->showSelectGamesDialog();
        }

        void NonOriginGameViewController::onShowPropertiesDialog(Engine::Content::EntitlementRef game)
        {
            if (!game.isNull())
            {
                mNonOriginGameView->showPropertiesDialog(game);
            }
        }

        void NonOriginGameViewController::onShowGameRepairDialog(Engine::Content::EntitlementRef game)
        {
            mShowingRepairDialog = true;
            mNonOriginGameView->showGameRepairDialog(game->contentConfiguration()->displayName());
        }

        void NonOriginGameViewController::onShowConfirmRemoveGame(Engine::Content::EntitlementRef game)
        {
            UIToolkit::OriginWindow* msgBox = mNonOriginGameView->createConfirmRemoveDialog(game->contentConfiguration()->displayName());
            ORIGIN_VERIFY_CONNECT(msgBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onRemoveGameConfirmed()));

            mNonOriginGameView->showConfirmRemoveDialog();
        }

        void  NonOriginGameViewController::onRemoveGameConfirmed()
        {
            mParent->onRemoveConfirmed();
        }

        void NonOriginGameViewController::onBrowseForGames()
        {
            QString startDir;

            #if defined(ORIGIN_PC)
                WCHAR buffer[MAX_PATH];
                SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, SHGFP_TYPE_CURRENT, buffer );
                startDir = QString::fromWCharArray(buffer);
            #elif defined(ORIGIN_MAC)
                startDir = Origin::Services::PlatformService::getStorageLocation(QStandardPaths::ApplicationsLocation);
            #else
                #error Must specialize for other platform.
            #endif

            QStringList choices;
            if (!mShowingRepairDialog)
            {                
                choices = QFileDialog::getOpenFileNames(EbisuSystemTray::instance()->primaryWindow(), "", startDir);
            }
            else
            {
                QString choice = QFileDialog::getOpenFileName(EbisuSystemTray::instance()->primaryWindow(), "", startDir);
                if (!choice.isNull()) {
                    choices.push_back(choice);
                }
                mShowingRepairDialog = false;
            }

            if (!choices.isEmpty())
            {
                mParent->onGameSelectionComplete(choices);
            }
            else
            {
                mParent->cancel();
            }
        }

        void NonOriginGameViewController::onScanForGames()
        {

        }

        void NonOriginGameViewController::onGameAlreadyExists(const QString& gameName)
        {
            mNonOriginGameView->showGameAlreadyAddedDialog(gameName);
        }

        void NonOriginGameViewController::onInvalidFileType(const QString& fileName)
        {
            mNonOriginGameView->showInvalidFileTypeDialog(fileName);
        }
    }
}
