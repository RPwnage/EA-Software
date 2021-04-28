package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test which download and launch DiP Small in Origin client, or check if DiP
 * Small in Game Library in browser.
 */
public class OABasicDownload extends EAXVxTestTemplate {

    @TestRail(caseId = 3121261)
    @Test(groups = {"quick_smoke", "long_smoke", "client_only"})
    public void testBasicDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Login to Origin"); // 1
        logFlowPoint("Navigate to game library"); // 2
        logFlowPoint("Open Slideout for " + entitlement.getName() + " and Verify it's Downloadable"); // 3
        logFlowPoint("Verify There is a Visible Navigation Section"); // 4
        logFlowPoint("Close the Slideout and Download " + entitlement.getName()); // 5
        logFlowPoint("Launch " + entitlement.getName()); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigated to Game Library.");
        } else {
            logFailExit("Could not navigate to Game Library.");
        }

        // 3
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        GameSlideout slideout = gameTile.openGameSlideout();
        if (slideout.verifyDownloadableState(3)) {
            logPass(entitlement.getName() + " is downloadable on the Slideout.");
        } else {
            logFail(entitlement.getName() + " is not downloadable on the Slideout.");
        }

        // 4
        if (slideout.verifyNavLinksVisible()) {
            logPass("Navigation section is visible.");
        } else {
            logFail("No visible navigation section exists.");
        }

        // 5
        slideout.clickSlideoutCloseButton();
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (downloaded) {
            logPass("Successfully downloaded " + entitlement.getName());
        } else {
            logFailExit("Could not download" + entitlement.getName());
        }

        // 6
        gameTile.play();
        if (entitlement.isLaunched(client)) {
            logPass("Successfully launched " + entitlement.getName());
        } else {
            logFailExit("Could not launch " + entitlement.getName());
        }
        softAssertAll();
    }
}
