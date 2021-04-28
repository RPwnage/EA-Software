package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadManager;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
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
 * Test download queue flyout by pausing the current download and re-logining in
 *
 * @author palui
 */
public class OADownloadQueuePauseLogout extends EAXVxTestTemplate {

    @TestRail(caseId = 9875)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadQueuePauseLogout(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);

        final String entitlementName = entitlement.getName();
        final String offerId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, entitlementName));//1
        logFlowPoint(String.format("Navigate to the Game Library. Verify '%s' appears as downloadable", entitlementName)); //2
        logFlowPoint(String.format("Start downloading '%s'. Verify 'Download Queue' shows it is currently downloading", entitlementName)); //3
        logFlowPoint(String.format("Click pause button at the 'Download Queue' tile for '%s'", entitlementName)); //4
        logFlowPoint("Logout and log back in"); //5
        logFlowPoint(String.format("Verify 'Download Queue' still shows '%s' as paused", entitlementName)); //6

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTile = new GameTile(driver, offerId);
        if (gameTile.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with downloadable game '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or cannot locate downloable game '%s'", entitlementName));
        }

        //3
        boolean startDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, offerId);
        DownloadQueueTile queueTile = new DownloadQueueTile(driver, offerId);
        new DownloadManager(driver).clickRightArrowToOpenFlyout();    
        boolean entitlementDownloading = Waits.pollingWait(queueTile::isGameDownloading);
        if (startDownloading && entitlementDownloading) {
            logPass(String.format("Verified start downloading successful and 'Download Queue' shows '%s' is currently downloading", entitlementName));
        } else {
            logFailExit(String.format("Failed: Start downloading failed or 'Download Queue' does not show '%s' is currently downloading", entitlementName));
        }

        //4
        sleep(5000); // Pause for some downloading to happen
        queueTile.clickPauseButton();
        if (Waits.pollingWait(queueTile::isPaused)) {
            logPass(String.format("Verified 'Download Queue' shows %s' as paused", entitlementName));
        } else {
            logFailExit(String.format("Failed: 'Download Queue' does not show '%s' as paused", entitlementName));
        }

        //5
        new MainMenu(driver).selectLogOut();
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified user can logout and log back in");
        } else {
            logFailExit("Failed: User cannot logout and log back in");
        }

        //6
        new DownloadManager(driver).openDownloadQueueFlyout();
        if (queueTile.isPaused()) {
            logPass(String.format("Verified after re-login, 'Download Queue' still shows %s' as paused", entitlementName));
        } else {
            logFailExit(String.format("Failed: After re-login, 'Download Queue' no longer shows '%s' as paused", entitlementName));
        }

        softAssertAll();
    }
}
