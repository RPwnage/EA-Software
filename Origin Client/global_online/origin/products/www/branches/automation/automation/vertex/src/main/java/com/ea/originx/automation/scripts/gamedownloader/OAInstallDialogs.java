package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadInfoDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Represents the super class for OAInstallDialogsDip and OAInstallDialogsNonDip
 * test cases dipSmall = True : Flow for OAInstallDialogsDip dipSmall = False :
 * Flow for OAInstallDialogsNonDip
 *
 * @author mkalaivanan
 */
public abstract class OAInstallDialogs extends EAXVxTestTemplate {

    /**
     * Parameterized Test method for the install dialog test cases
     *
     * @param context
     * @param dipSmall = true - Flow for OAInstallDialogsDip dipSmall = False -
     *                 Flow for OAInstallDialogsNonDip
     * @param testName The name of the test. We need to pass this up so
     *                 initLogger properly attaches to the CRS test case.
     * @throws Exception
     */
    public void testInstallDialog(ITestContext context, boolean dipSmall, String testName) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = dipSmall ? new OADipSmallGame() : EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementDownloadSize = entitlement.getDownloadSize();
        final String entitlementInstallSize = entitlement.getInstallSize();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();

        logFlowPoint("Login to Origin as user: " + username); //1
        logFlowPoint("Navigate to game library"); //2
        if (dipSmall) {
            logFlowPoint("Start Downloading Client Automation DiP small; Verify the Language Selection Dialog Appear"); //3a
            logFlowPoint("Verify the Language Selection Dialog Appears Correctly and Defaults to English"); //4a
            logFlowPoint("Press 'Accept' and Verify Language Selection Dialog is completed"); //5a
        }
        logFlowPoint("Start Downloading " + entitlementName + "; Verify the Download Dialog Appears"); //6
        logFlowPoint("Verify the Download and Install Sizes Appear and are Correct"); //7
        logFlowPoint("Verify the Start Menu and Desktop Shortcut Checkboxes Appear and are Checked"); //8
        if (dipSmall) {
            logFlowPoint("Press 'Next' and Verify the EULA Acceptance Dialog Appears"); //9a
            logFlowPoint("Verify the User Cannot Continue Without Accepting the EULA"); //10a
            logFlowPoint("Verify Download completed:Accept the EULA and Verify the User May Continue"); //11a
        } else {
            logFlowPoint("Press 'Next' and Verify the Entitlement Downloads and is Ready to Install"); //9b
            logFlowPoint("Press 'Install' and Verify the Entitlement Begins Installing"); //10b
            logFlowPoint("Verify " + entitlementName + " successfully Downloads and Installs");//11b
        }
        logFlowPoint("Launch entitlement: " + entitlementName); //12

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        sleep(2000);
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigate to game library");
        } else {
            logFailExit("Could not navigate to game library");
        }

        final GameTile gameTile = new GameTile(driver, entitlementOfferId);
        final GameSlideout gameSlideout = gameTile.openGameSlideout();
        gameSlideout.startDownload(OriginClientData.DOWNLOAD_INITIAL_TIMEOUT_SEC);
        DownloadInfoDialog downloadInfo;
        DownloadLanguageSelectionDialog downloadLanguageSelectionDialog;
        if (dipSmall) {
            downloadLanguageSelectionDialog = new DownloadLanguageSelectionDialog(driver, OriginClientData.DOWNLOAD_PREPARATION_TIMEOUT_SEC);
            //3a
            if (downloadLanguageSelectionDialog.waitIsVisible()) {
                logPass("Opened Select Language dialog");
            } else {
                logFailExit("Could not start the download or the Select Language dialog did not open");
            }
            //4a
            boolean defaultEnglishSelectionExists = downloadLanguageSelectionDialog.verifyEnglishLanguageSelectionExists();
            if (defaultEnglishSelectionExists) {
                logPass("Selection dialog defaults to English GB");
            } else {
                logFailExit("Selection dialog did not default to English GB");
            }
            //5a
            downloadLanguageSelectionDialog.clickAccept();
            sleep(1500);
            boolean verifyDownloadLanguageSelectionNotExists = !downloadLanguageSelectionDialog.waitIsVisible();
            if (verifyDownloadLanguageSelectionNotExists) {
                logPass("Successfully complete language selection dialog");
            } else {
                logFailExit("Could not complete language selection dialog");
            }

        }
        //6
        downloadInfo = new DownloadInfoDialog(driver);
        if (downloadInfo.waitIsVisible()) {
            logPass("Download Dialog appeared after download started");
        } else {
            logFailExit("Download Dialog did not appear");
        }

        //7
        boolean entitlementDownloadSizeExsists = downloadInfo.verifyDownloadOrInstallSize(entitlementDownloadSize);
        boolean entitlementInstallSizeExsists = downloadInfo.verifyInstallOrDiskSpaceAvailableSize(entitlementInstallSize);
        if (entitlementDownloadSizeExsists && entitlementInstallSizeExsists) {
            logPass("Download size and install size are displayed on the dialog with correct information");
        } else {
            logFailExit("Download size and install size are not displayed on the dialog");
        }

        //8
        boolean desktopShortcutBoxLabelTextExists = downloadInfo.verifyDesktopShortcutLabel();
        boolean startMenuShortcutBoxLabelTextExists = downloadInfo.verifyStartMenuShortcutLabel();
        boolean checkBoxesDefaultChecked = downloadInfo.verifyDesktopAndStartMenuCheckBoxesChecked();
        if (desktopShortcutBoxLabelTextExists && startMenuShortcutBoxLabelTextExists && checkBoxesDefaultChecked) {
            logPass("Shortcut checkboxes and labels all exist");
        } else {
            logFailExit("Shortcut checkboxes or labels do not exist");
        }

        downloadInfo.clickNext();
        sleep(1000);
        if (dipSmall) {
            DownloadEULASummaryDialog downloadEULASummary = new DownloadEULASummaryDialog(driver);
            //9a
            if (downloadEULASummary.isOpen() && downloadEULASummary.verifyEULAAcceptanceBoxExists()) {
                logPass("Successfully accepted the download confirmation dialog and opened the EULA dialog");
            } else {
                logFailExit("Could not complete the download confirmation dialog or open the EULA dialog");
            }
            //10a
            boolean verifyNextButtonDisabled = !downloadEULASummary.verifyNextButtonEnabled();
            if (verifyNextButtonDisabled && downloadEULASummary.verifyNextButtonNotClickable()) {
                logPass("Next button is disabled; the user is unable to continue without accepting the EULA");
            } else {
                logFailExit("Next button is not disabled");
            }
            //11a
            downloadEULASummary.setLicenseAcceptCheckbox(true);
            sleep(500);//small sleep as it sometimes fails by the time checkbox is checked ,click Next button happens which is only enabled after checkbox is checked
            downloadEULASummary.clickNextButton();
            if (gameSlideout.verifyPlayableState(OriginClientData.GAME_LAUNCH_TIMEOUT_SEC)) {
                logPass("Download Complete : Accepted EULA checkbox ,completed EULA dialog");
            } else {
                logFailExit("Download failed");
            }
            //12
            gameSlideout.startPlay(OriginClientData.INSTALLATION_TIMEOUT_SEC);
            new OADipSmallGame().waitForGameLaunch(client);
            Waits.pollingWaitEx(() -> entitlement.isLaunched(client));
            if (gameSlideout.verifyPlayingState(OriginClientData.GAME_LAUNCH_TIMEOUT_SEC) && entitlement.isLaunched(client)) {
                logPass("Launch " + entitlementName + " successful");
            } else {
                logFailExit("Launch " + entitlementName + " unsuccessful");
            }

        } else {
            //9b
            if (gameSlideout.verifyInstallState(OriginClientData.INSTALLATION_PREPARATION_SEC)) {
                logPass("Successfully accepted the download confirmation and got to installation");
            } else {

                logFailExit("Failed to be in Ready to be installed state");
            }
            //10b
            sleep(2000);
            gameSlideout.startInstall(OriginClientData.INSTALLATION_PREPARATION_SEC);
            if (gameSlideout.verifyFinalizingState(OriginClientData.INSTALLATION_TIMEOUT_SEC)) {
                logPass("Started installing the entitlement");
            } else {
                logFailExit("Could not begin installing the entitlement");
            }

            Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, OriginClientData.DEFAULT_GAME_INSTALLER));
            ProcessUtil.killProcess(client, OriginClientData.DEFAULT_GAME_INSTALLER);
            //11b
            if (gameSlideout.verifyPlayableState(OriginClientData.GAME_LAUNCH_TIMEOUT_SEC)) {
                logPass("Completed downloading " + entitlementName);
            } else {
                logFailExit("Download did not complete");
            }

            //12
            gameSlideout.startPlay(OriginClientData.INSTALLATION_TIMEOUT_SEC);

            Waits.pollingWaitEx(() -> entitlement.isLaunched(client));
            if (gameSlideout.verifyPlayingState(OriginClientData.GAME_LAUNCH_TIMEOUT_SEC) && entitlement.isLaunched(client)) {
                logPass("Launch " + entitlementName + " successful");
            } else {
                logFailExit("Launch " + entitlementName + " unsuccessful");
            }
        }

        softAssertAll();
    }

}
