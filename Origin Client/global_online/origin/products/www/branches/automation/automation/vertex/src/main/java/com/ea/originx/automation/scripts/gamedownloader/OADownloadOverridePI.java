package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.resources.EACore;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the progressive install feature and the stepped download features
 *
 * @author lscholte
 */
public class OADownloadOverridePI extends EAXVxTestTemplate {

    @TestRail(caseId = 9610)
    @Test(groups = {"gamedownloader", "client_only", "full_regression", "int_only"})
    public void testDownloadOverridePI(ITestContext context) throws Exception {

        UserAccount user = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        
        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_15);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        final String originProcess = OriginClientData.ORIGIN_PROCESS_NAME;
        
        client.getEACore().setEACoreValue(EACore.EACORE_PROGRESSIVE_INSTALL_SECTION,
                EACore.EACORE_ENABLE_PROGRESSIVE_INSTALL,
                EACore.EACORE_TRUE);
        client.getEACore().setEACoreValue(EACore.EACORE_PROGRESSIVE_INSTALL_SECTION,
                EACore.EACORE_ENABLE_STEPPED_DOWNLOAD,
                EACore.EACORE_TRUE);

        logFlowPoint("Log into Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Begin downloading of " + entitlementName); //3
        logFlowPoint("Verify overlay for " + entitlementName + " indicates it is playable before reaching 100%"); //4
        logFlowPoint("Cancel downloading of " + entitlementName); //5
        logFlowPoint("Set EnableProgressiveInstall=false in the EACore"); //6
        logFlowPoint("Exit the Origin client, relaunch and sign back in with user " + user.getUsername()); //7
        logFlowPoint("Begin downloading of " + entitlementName); //8
        logFlowPoint("Verify overlay for " + entitlementName + " does not indicate it is playable before reaching 100%"); //9
        logFlowPoint("Cancel downloading of " + entitlementName); //10
        logFlowPoint("Set EnableProgressiveInstall=true in the EACore"); //11
        logFlowPoint("Exit the Origin client, relaunch and sign back in with user " + user.getUsername()); //12
        logFlowPoint("Begin downloading of " + entitlementName); //13
        logFlowPoint("Verify that the download for " + entitlementName + " pauses after the game becomes playable"); //14
        logFlowPoint("Cancel the downloading of " + entitlementName); //15
        logFlowPoint("Set EnableSteppedDownload=false in the EACore"); //16
        logFlowPoint("Exit the Origin client, relaunch and sign back in with user " + user.getUsername()); //17
        logFlowPoint("Begin downloading of " + entitlementName); //18
        logFlowPoint("Verify that the game continues to download after becoming playable and does not pause"); //19

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        MacroPurchase.purchaseEntitlement(driver, entitlement);
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        boolean startedDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId);
        logPassFail(startedDownloading, true);

        //4
        GameTile gameTile1 = new GameTile(driver, entitlementOfferId);
        // Verify the text before the game has reached a playable point
        boolean overlayPlayableStatusPercent = gameTile1.verifyOverlayPlayableStatusPercent();
        // Verify the offer reaches a state where it is both downloading and playable at the same time
        boolean playableAndDownloading = Waits.pollingWait(() -> gameTile1.isDownloading() && gameTile1.isReadyToPlay(),
                600000, 2000, 0);
        logPassFail((overlayPlayableStatusPercent && playableAndDownloading), false);

        //5
        gameTile1.cancelDownload();
        CancelDownload cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        logPassFail(gameTile1.waitForDownloadable(), true);
        
        //6
        boolean eaCoreModified = client.getEACore().setEACoreValue(EACore.EACORE_PROGRESSIVE_INSTALL_SECTION,
                EACore.EACORE_ENABLE_PROGRESSIVE_INSTALL,
                EACore.EACORE_FALSE);
        logPassFail(eaCoreModified, true);
        
        //7
        new MainMenu(driver).selectExit();
        client.stop();
        ProcessUtil.killProcess(client, originProcess);
        driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);
        
        //8
        //Throttle the download speed to ensure that the game doesn't complete it's installation for this test
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        appSettings.verifyAppSettingsReached();
        appSettings.setDownloadThrottleOutOfGame("1000000");
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        gameLibrary.verifyGameLibraryPageReached();
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId), true);
        
        //9
        GameTile gameTile2 = new GameTile(driver, entitlementOfferId);
        logPassFail(!gameTile2.verifyOverlayPlayableStatusPercent(), false);
        
        //10
        gameTile2.cancelDownload();
        cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        logPassFail(gameTile2.waitForDownloadable(), true);
        
        //11
        eaCoreModified = client.getEACore().setEACoreValue(EACore.EACORE_PROGRESSIVE_INSTALL_SECTION,
                EACore.EACORE_ENABLE_PROGRESSIVE_INSTALL,
                EACore.EACORE_TRUE);
        logPassFail(eaCoreModified, true);
        
        //12
        new MainMenu(driver).selectApplicationSettings();
        appSettings.verifyAppSettingsReached();
        appSettings.setDownloadThrottleOutOfGame("0");
        new MainMenu(driver).selectExit();
        client.stop();
        ProcessUtil.killProcess(client, originProcess);
        driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);
        
        //13
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        gameLibrary.verifyGameLibraryPageReached();
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId), true);
        
        //14
        GameTile gameTile3 = new GameTile(driver, entitlementOfferId);
        Waits.pollingWait(() -> gameTile3.isDownloading() && gameTile3.isReadyToPlay(),
                600000, 2000, 0);
        //Small wait to ensure we get the right download percentage
        sleep(1000);
        int initialDownloadPercentage = gameTile3.getDownloadPercentage();
        //Pause the test for 30 seconds before checking the download percentage again to prove that the download has been paused
        sleep(30000);
        int finalDownloadPercentage = gameTile3.getDownloadPercentage();
        logPassFail(initialDownloadPercentage==finalDownloadPercentage, false);
        
        //15
        gameTile3.cancelDownload();
        cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        logPassFail(gameTile3.waitForDownloadable(), true);
        
        //16
        eaCoreModified = client.getEACore().setEACoreValue(EACore.EACORE_PROGRESSIVE_INSTALL_SECTION,
                EACore.EACORE_ENABLE_STEPPED_DOWNLOAD,
                EACore.EACORE_FALSE);
        logPassFail(eaCoreModified, true);
        
        //17
        new MainMenu(driver).selectExit();
        client.stop();
        ProcessUtil.killProcess(client, originProcess);
        driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);
        
        //18
        navBar = new NavigationSidebar(driver);
        gameLibrary = navBar.gotoGameLibrary();
        gameLibrary.verifyGameLibraryPageReached();
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId), true);
        
        //19
        GameTile gameTile4 = new GameTile(driver, entitlementOfferId);
        Waits.pollingWait(() -> gameTile4.isDownloading() && gameTile4.isReadyToPlay(),
                600000, 2000, 0);
        final int downloadPercentageAtPlayable = gameTile4.getDownloadPercentage();
        boolean downloadProgresses = Waits.pollingWait(() -> gameTile4.getDownloadPercentage() > downloadPercentageAtPlayable,
                180000, 2000, 0);
        logPassFail(downloadProgresses, false);
        //Cancelling the download to allow for more stable stress tests
        gameTile4.cancelDownload();
        cancelDialog = new CancelDownload(driver);
        cancelDialog.isOpen();
        cancelDialog.clickYes();
        
        softAssertAll();
    }
}