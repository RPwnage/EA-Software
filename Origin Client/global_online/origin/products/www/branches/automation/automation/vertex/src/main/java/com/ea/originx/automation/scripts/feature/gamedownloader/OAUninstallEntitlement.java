package com.ea.originx.automation.scripts.feature.gamedownloader;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.FileUtil;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test installing a DiP entitlement and then un-installing it
 *
 * @author palui
 */
public class OAUninstallEntitlement extends EAXVxTestTemplate {

    @TestRail(caseId = 3087350)
    @Test(groups = {"gamelibrary", "gamedownloader_smoke", "gamedownloader", "client_only", "allfeaturesmoke"})
    public void testUpdateRestrictGameFolder(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String offerId = entitlement.getOfferId();

        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();
        final String executablePath = entitlement.getExecutablePath(client);
        final String entitlementFolder = entitlement.getDirectoryName();

        logFlowPoint("Launch Origin and login as a user entitled with an entitlement"); //1
        logFlowPoint("Navigate to the 'Game Library' and verify the entitlement can be downloaded"); //2
        logFlowPoint("Download and install the entitlement, and verify that it appears as installed in the 'Game Library'"); //3
        logFlowPoint("Verify that the entitlement has been added to the 'Origin Games' folder"); //4
        logFlowPoint("Un-install the entitlement and verify that it appears as un-installed in the 'Game Library'"); //5
        logFlowPoint("Verify that the entitlement and its related files have been removed from the 'Origin Games' folder"); //6

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary().waitForPage();
        if (new GameTile(driver, offerId).waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to 'Game Library' with downloadable '%s' game tile", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to 'Game Library', or cannot locate downloadable '%s' game tile", entitlementName));
        }

        //3
        if (MacroGameLibrary.downloadFullEntitlement(driver, offerId)) {
            logPass(String.format("Verified '%s' appears as installed in the 'Game Library'", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot download '%s' or it does not appear as installed in the 'Game Library'", entitlementName));
        }

        //4
        if (FileUtil.isFileExist(client,executablePath)) {
            logPass(String.format("Verified '%s' files have been added to the 'Origin Games' folder", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' files have not been added to the 'Origin Games' folder", entitlementName));
        }

        //5
        entitlement.enableSilentUninstall(client);
        GameTile gameTile = new GameTile(driver, offerId);
        gameTile.uninstall();
        boolean gameReadyToDownload = gameTile.waitForDownloadable();
        boolean gameUninstalled = !gameTile.isReadyToPlay();
        if (gameReadyToDownload && gameUninstalled) {
            logPass(String.format("Verified '%s' appears as un-installed in the 'Game Library'", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' does not appear as un-installed in the 'Game Library'", entitlementName));
        }

        //6
        if (!FileUtil.isDirectoryExist(client,entitlementFolder)) {
            logPass(String.format("Verified '%s' file have been removed from the 'Origin Games' folder", entitlementName));
        } else {
            logFailExit(String.format("Failed: '%s' files have not been removed form the 'Origin Games' folder", entitlementName));
        }

        softAssertAll();
    }
}
