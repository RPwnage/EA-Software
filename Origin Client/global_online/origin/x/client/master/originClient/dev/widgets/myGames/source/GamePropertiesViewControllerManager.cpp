/////////////////////////////////////////////////////////////////////////////
// GamePropertiesViewControllerManager.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "GamePropertiesViewControllerManager.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/LocalContent.h"
#include "engine/content/LocalInstallProperties.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "ui_GamePropertiesView.h"
#include "engine/content/GamesController.h"
#include "TelemetryAPIDLL.h"
#include "InstallerView.h"

namespace Origin
{
namespace Client
{

GamePropertiesViewController::GamePropertiesViewController(Engine::Content::EntitlementRef game, QWidget* parent)
: QWidget(parent)
, mGame(game)
, ui(new Ui::GamePropertiesView())
{
    ui->setupUi(this);
    ui->langOptions->hide();
    const QString productId = mGame->contentConfiguration()->productId();
    Engine::Content::GamesController* gamesController = Engine::Content::GamesController::currentUserGamesController();
    if(gamesController)
    {
        // Launch options

        QStringList launchTitles = mGame->localContent()->launchTitleList();
        QString title = gamesController->gameMultiLaunchDefault(productId);

        if(title == "")
            title = mGame->localContent()->defaultLauncher();

        if(title != Engine::Content::GamesController::SHOW_ALL_TITLES && title != "" && launchTitles.contains(title))
        {
            // Find it in the current list and move it to the top.
            launchTitles.move(launchTitles.indexOf(title), 0);
        }

        if(launchTitles.length() < 2)
        {
            ui->launchOptions->setVisible(false);
            ui->launchOptions->setEnabled(false);
        }
        else
        {
            ui->ddLaunchOptions->addItem(tr("ebisu_client_present_all_options"));
            ui->ddLaunchOptions->addItems(launchTitles);

            mInitMultiLaunchValue = title;
            const int index = ui->ddLaunchOptions->findText(mInitMultiLaunchValue);
            if(mInitMultiLaunchValue != "" && index != -1)
            {
                ui->ddLaunchOptions->setCurrentIndex(index);
            }
        }

        // OIG Disabled
        mInitOIGDisabledValue = gamesController->gameHasOIGManuallyDisabled(productId);
        ui->cbDisableOIG->setChecked(mInitOIGDisabledValue);
        ui->cbDisableOIG->setVisible(mGame->contentConfiguration()->isFreeTrial() == false);

        mInitCommandLineArgValue = gamesController->gameCommandLineArguments(productId);
        ui->leCommandLineArg->setText(mInitCommandLineArgValue);

#if defined(ORIGIN_MAC)
        // Add 10 to the spacing on Mac to match Windows UI
        ui->ddLaunchOptions->adjustSize();
        ui->ddLaunchOptions->setFixedWidth(ui->ddLaunchOptions->width() + 10);
#endif
    }

    if(mGame->localContent() && mGame->localContent()->localInstallProperties())
    {
        Origin::Downloader::InstallLanguageRequest langRequest;
        langRequest.availableLanguages = mGame->localContent()->availableLocales(mGame->localContent()->localInstallProperties()->GetAvailableLocales(), 
                                                                                 mGame->contentConfiguration()->relatedGameContentIds(), 
                                                                                 mGame->localContent()->localInstallProperties()->GetManifestVersion().ToStr());
        if(mGame->contentConfiguration()->softwareBuildMetadataPresent() == NULL
            || mGame->contentConfiguration()->softwareBuildMetadata().mbLanguageChangeSupportEnabled == false)
        {
            ORIGIN_LOG_EVENT << "GP: Language change not supported - no feature flag";
        }
        else if(langRequest.availableLanguages.size() < 2)
        {
            ORIGIN_LOG_EVENT << "GP: Language change not supported - only one locale";
        }
        else
        {
            mInitLang = mGame->localContent()->installedLocale();
            delete ui->ddLangOptions;
            UIToolkit::OriginDropdown* dropdown = dynamic_cast<UIToolkit::OriginDropdown*>(InstallerView::languageSelectionContent(langRequest, mInitLang));
            if(dropdown)
            {
                ui->ddLangOptions = dropdown;
                ui->ddLangOptions->setCurrentText(mInitLang);
                ui->langOptionsLayout->addWidget(ui->ddLangOptions);
                ui->langOptions->show();
                ORIGIN_LOG_EVENT << "GP: Language change - showing";
            }
        }
    }
}


GamePropertiesViewController::~GamePropertiesViewController()
{
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void GamePropertiesViewController::onApplyChanges()
{
    Engine::Content::GamesController* gamesController = Engine::Content::GamesController::currentUserGamesController();
    const QString productId = mGame->contentConfiguration()->productId();
    if(gamesController)
    {
        // Launch options
        QString title = ui->ddLaunchOptions->currentText();
        if(ui->launchOptions->isEnabled() 
            && mInitMultiLaunchValue.compare(title) != 0)
        {
            if(title == tr("ebisu_client_present_all_options"))
            {
                title = Engine::Content::GamesController::SHOW_ALL_TITLES;
            }
            gamesController->setGameMultiLaunchDefault(productId, title);
        }

        bool igoDisabledChanged = false;
        bool exeParamsChanged = false;

        // Manually disable OIG
        if(mInitOIGDisabledValue != ui->cbDisableOIG->isChecked())
        {
            gamesController->setGameHasOIGManuallyDisabled(productId, ui->cbDisableOIG->isChecked());
            igoDisabledChanged = true;
        }

        if(mInitCommandLineArgValue.compare(ui->leCommandLineArg->text()) != 0)
        {
            gamesController->setGameCommandLineArguments(productId, ui->leCommandLineArg->text());
            exeParamsChanged = true;
        }

        if( igoDisabledChanged || exeParamsChanged)
        {
            // truncate the exe params to 256 characters
            QString exeParamsTruncated = gamesController->gameCommandLineArguments(productId);
            exeParamsTruncated.truncate(256);

            // applied changes to Origin Game properties
            GetTelemetryInterface()->Metric_GAMEPROPERTIES_APPLY_CHANGES(
                false,
                productId.toUtf8().data(),
                QString().toUtf8().data(),
                exeParamsTruncated.toUtf8().data(),
                gamesController->gameHasOIGManuallyDisabled(productId));
        }
    }

    const QString locale = ui->ddLangOptions->itemData(ui->ddLangOptions->currentIndex()).toString();
    if(mGame->localContent() && mInitLang != locale)
    {
        GetTelemetryInterface()->Metric_GAMEPROPERTIES_LANG_CHANGES(productId.toUtf8().data(), locale.toUtf8().data());
        mGame->localContent()->setPreviousInstalledLocale(mInitLang);
        mGame->localContent()->setInstalledLocale(locale);
        mGame->localContent()->repair(true);
    }
}

}
}
