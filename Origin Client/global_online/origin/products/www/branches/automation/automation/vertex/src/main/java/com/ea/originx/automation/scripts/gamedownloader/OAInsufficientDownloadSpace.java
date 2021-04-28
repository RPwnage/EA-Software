package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadInfoDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests the insufficient download space for an entitlement
 *
 * @author mkalaivanan
 * @author mdobre
 */
public class OAInsufficientDownloadSpace extends EAXVxTestTemplate {

    public void testInSufficientDownloadSpace(ITestContext context, boolean isDIP) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String gameName;
        final String gameOfferId;
        final String gameInstallSize;
        final String downloadLocation;
        final String virtualDiskFolder = OriginClientConstants.VIRTUAL_DISK_LOCATION;
        if(isDIP) {
            gameName = EntitlementInfoConstants.DIP_LARGE_NAME;
            gameOfferId = EntitlementInfoConstants.DIP_LARGE_OFFER_ID;
            gameInstallSize = "";
            downloadLocation = OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER + ":\\Games\\";
        }
        else {
            gameName = EntitlementInfoConstants.NON_DIP_LARGE_NAME;
            gameOfferId = EntitlementInfoConstants.NON_DIP_LARGE_OFFER_ID;
            gameInstallSize = EntitlementInfoConstants.NON_DIP_LARGE_INSTALL_SIZE;
            downloadLocation = OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER + ":\\Installer\\";
        }

        final String virtualDriveDownloadFolder = downloadLocation;

        final int diskSize = 100;

        final Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(gameName);
        UserAccount userAccount = AccountManager.getInstance().requestWithCriteria(criteria, 360);

        logFlowPoint("Create a partition on the disk of size 100MB and assign to W: Drive"); //1
        logFlowPoint("Create a folder in the drive to direct the downloads to"); //2
        logFlowPoint("Launch and login to Origin with: " + userAccount.getUsername()); //3
        logFlowPoint("Navigate to Settings"); //4
        logFlowPoint("Navigate to Install & Saves tab"); //5
        if(isDIP)
            logFlowPoint("Change Game Download location to " + virtualDriveDownloadFolder); //6
        else
            logFlowPoint("Change Game Installer location to " + virtualDriveDownloadFolder); //6
        logFlowPoint("Navigate to Game Library"); //7
        if(isDIP) {
            logFlowPoint("Begin downloading: " + gameName + " and Verify 'Insufficient Download Space' message appears"); //8
            logFlowPoint("Verify disk space availability is displayed to the User"); //9
            logFlowPoint("Verify user cannot download until enough disk space is available"); //10
        }
        else {
            logFlowPoint("Begin downloading " + gameName); //8
            logFlowPoint("Verify disk space availability is displayed to the User"); //9
            logFlowPoint("Verify user cannot download until enough disk space is available"); //10
        }

        // In case the clean up process did not delete and detach the hard drive and failed half way through it
        FileUtil.deleteVirtualHardDrive(client);
        FileUtil.deleteFolder(client, virtualDiskFolder);
        FileUtil.createFolder(client, virtualDiskFolder);  

        //1
        if (FileUtil.createVirtualHardDrive(client, virtualDiskFolder, OriginClientConstants.NEW_VIRTUAL_DRIVE_LETTER, diskSize)) {
            logPass("Successfully create virtual drive:W:\\  of size 100MB ");
        } else {
            logFailExit("Unable to create virtual drive:W:\\ of size 100MB");
        }
        
        //2
        sleep(3000); //wait for virtual drive to be created
        if(FileUtil.createFolder(client, virtualDiskFolder)) {
            logPass("Successfully create folder in virtual drive W:");
        }
        else {
            logFailExit("Unable to create folder in virtual drive W:");
        }

        //3
        // Use single instance client otherwise a new Origin instance starts when launching a game outside of Origin
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user account: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user account: " + userAccount.getUsername());
        }
        
        //4
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page");
        }
        
        //5
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);

        if (Waits.pollingWait(installSaveSettings::verifyInstallSaveSettingsReached)) {
            logPass("Successfully navigated to 'Install & Saves' section");
        } else {
            logFailExit("Failed to navigate to 'Install & Saves' section");
        }
        
        //6
        boolean gameDownloadLocationChanged;
        if(isDIP) {
            gameDownloadLocationChanged = installSaveSettings.changeGamesLocationTo(virtualDriveDownloadFolder);
        }
        else {
            gameDownloadLocationChanged = installSaveSettings.changeInstallerLocationTo(virtualDriveDownloadFolder);
        }
        if (gameDownloadLocationChanged) {
            logPass("Changed Download locations to W:");
        } else {
            logFail("Download Location changes to W: failed");
        }
        
        //7
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }
        
        if(isDIP) {
            //8
            GameTile gameTile = new GameTile(driver, gameOfferId);
            gameTile.startDownload();

            DownloadInfoDialog downloadInfoDialogDipLarge = new DownloadInfoDialog(driver);
            Waits.pollingWait(downloadInfoDialogDipLarge::isOpen, 30000, 1000, 0); // Use longer wait than waitForVisible default
            if (Waits.pollingWait(downloadInfoDialogDipLarge::verifyInsufficientSpaceWarningMessage)) {
                logPass("Insufficient Download Space message appeared successfully after trying to download " + gameName);
            } else {
                logFail("Insufficient Download Space message failed to appear after trying to download " + gameName);
            }

            //9
            if (downloadInfoDialogDipLarge.verifyInstallOrDiskSpaceAvailableSize()) {
                logPass("Disk space availability is visible to the user");
            } else {
                logFail("Disk space availability is not visible to the user");
            }

            //10
            if (!downloadInfoDialogDipLarge.verifyNextButtonEnabled()) {
                logPass("User cannot proceed the download until enough disk space");
            } else {
                logFailExit("User can proceed the download with not enough disk space");
            }
        }
        else {
            //8
            GameTile gameTileNonDipLarge = new GameTile(driver, gameOfferId);
            gameTileNonDipLarge.startDownload();
            
            DownloadInfoDialog downloadInfoDialogNonDipLarge = new DownloadInfoDialog(driver);
            Waits.pollingWait(downloadInfoDialogNonDipLarge::isOpen, 30000, 1000, 0); // Use longer wait than waitForVisible default
            if (Waits.pollingWait(downloadInfoDialogNonDipLarge::verifyInsufficientSpaceWarningMessage)) {
                logPass("Insufficient Download Space message appeared successfully after trying to download " + gameName);
            } else {
                logFailExit("Insufficient Download Space message failed to appear after trying to download " + gameName);
            }
            
            //9
            if (downloadInfoDialogNonDipLarge.verifyInstallOrDiskSpaceAvailableSize()) {
                logPass("Disk space availability is displayed to the user");
            } else {
                logFail("Disk space availability is not displayed to the user");
            }

            //10
            if (!downloadInfoDialogNonDipLarge.verifyNextButtonEnabled()) {
                logPass("User cannot proceed the download until enough disk space is available");
            } else {
                logFailExit("User can proceed the download with not enough disk space");
            }
        }
        
        softAssertAll();
    }

}
