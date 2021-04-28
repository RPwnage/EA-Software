package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;

import java.lang.invoke.MethodHandles;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify network offline login of 2 users with different entitlements
 *
 * @author palui
 */
public class OANetworkOfflineDifferentUsers extends EAXVxTestTemplate {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    @TestRail(caseId = 39462)
    @Test(groups = {"loginregistration", "client_only", "full_regression"})
    public void testNetworkOfflineDifferentUsers(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlementA = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SDK_TOOL);
        EntitlementInfo entitlementB = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        EntitlementInfo upgradedEntitlementB = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        String entitlementNameA = entitlementA.getName();
        String entitlementNameB = entitlementB.getName();

        UserAccount userA = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENTS_SDK_TEST_ACCOUNT); // Owns SDK but not BF4
        UserAccount userB = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_UPGRADE_TEST); // Owns BF4 but not SDK
        String usernameA = userA.getUsername();
        String usernameB = userB.getUsername();

        logFlowPoint(String.format("Launch Origin client and login as UserA %s entitled to '%s'", usernameA, entitlementNameA)); //1
        logFlowPoint(String.format("Navigate to game library and verify it has '%s' but not '%s'", entitlementNameA, entitlementNameB)); //2
        logFlowPoint("Logout from UserA " + usernameA); //3
        logFlowPoint(String.format("Login as UserB %s entitled to '%s'", usernameB, entitlementNameB)); //4
        logFlowPoint(String.format("Navigate to game library and verify it has '%s' but not '%s'", entitlementNameB, entitlementNameA)); //5
        logFlowPoint(String.format("Login from UserB %s and exit Origin", usernameB)); //6
        logFlowPoint("Disconnect Origin client from the network"); //7
        logFlowPoint("Launch Origin and verify Login Window is in offline mode"); //8
        logFlowPoint("Login as UserA: " + usernameA); //9
        logFlowPoint(String.format("Navigate to game library and verify it has '%s' but not '%s'", entitlementNameA, entitlementNameB)); //10
        logFlowPoint(String.format("Logout from %s and verify Login window is again in offline mode", usernameA)); //11
        logFlowPoint("Login as UserB: " + usernameB); //12
        logFlowPoint(String.format("Navigate to game library and verify it has '%s' but not '%s'", entitlementNameB, entitlementNameA)); //13

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userA)) {
            logPass("Verified login successful as UserA " + usernameA);
        } else {
            logFailExit("Failed: Cannot login as UserA " + usernameA);
        }

        //2
        if (verifyLibraryGamesXNotY(driver, entitlementA, entitlementB)) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s' but not '%s'", entitlementNameA, entitlementNameB));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown, or '%s' shown", entitlementNameA, entitlementNameB));
        }

        //3
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage()) {
            logPass("Verified logout successful from UserA");
        } else {
            logFailExit("Failed: Cannot logout from UserA");
        }

        //4
        if (MacroLogin.startLogin(driver, userB)) {
            logPass("Verified login successful as UserB " + usernameB);
        } else {
            logFailExit("Failed: Cannot login as UserB " + usernameB);
        }

        //5
        // Tagged acount for subs upgrade test which may require cleanup
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        gameLibrary.waitForPage();
        if (gameLibrary.isGameTileVisibleByName(upgradedEntitlementB.getName(), 2)) {
            _log.warn(String.format("Downgrade required for %s (from %s)", entitlementNameB, upgradedEntitlementB.getName()));
            MacroGameLibrary.removeFromLibrary(driver, upgradedEntitlementB.getOfferId());
        }
        if (verifyLibraryGamesXNotY(driver, entitlementB, entitlementA)) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s' but not '%s'", entitlementNameB, entitlementNameA));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown, or '%s' shown", entitlementNameB, entitlementNameA));
        }

        //6
        // Cache update may take long time (15+ secs) to complete. Pause before logout
        // otherwise Origin may complaint login is for the first time
        sleep(35000);
        loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage()) {
            logPass("Verified logout successful from UserB");
        } else {
            logFailExit("Failed: Cannot logout from UserB");
        }
        //loginPage.close(); // Exit to get Login window to show offline mode
        client.stop();

        //7
        if (FirewallUtil.blockOrigin(client)) {
            logPass("Verified Origin is disconnected from the Network");
        } else {
            logFailExit("Failed: Origin is not disconnected from the Network");
        }

        //8
        driver = startClientObject(context, client);
        loginPage = new LoginPage(driver);
        if (loginPage.verifySiteStripeOfflineMessage()) {
            logPass("Verified Origin launches and Login Window is in offline mode");
        } else {
            logFailExit("Failed: Cannot launch Origin or Login window is not in offline mode");
        }

        //9
        if (MacroLogin.networkOfflineClientLogin(driver, userA)) {
            logPass(String.format("Verfied login (with Origin disconnected) successful as UserA %s", usernameA));
        } else {
            logFailExit(String.format("Failed: Cannot login (with Origin disconnected) as UserA %s", usernameA));
        }

        //10
        if (verifyLibraryGamesXNotY(driver, entitlementA, entitlementB)) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s' but not '%s'", entitlementNameA, entitlementNameB));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown, or '%s' shown", entitlementNameA, entitlementNameB));
        }

        //11
        loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage() && loginPage.verifySiteStripeOfflineMessage()) {
            logPass("Verified logout successful from UserA and Login window is again in offline mode");
        } else {
            logFailExit("Failed: Cannot logout from UserA or Login window is not in offline mode");
        }

        //12
        if (MacroLogin.startLogin(driver, userB)) {
            logPass("Verified login (with Origin disconnected) successful as UserB " + usernameB);
        } else {
            logFailExit("Failed: Cannot login (with Origin disconnected) as UserB " + usernameB);
        }

        //13
        if (verifyLibraryGamesXNotY(driver, entitlementB, entitlementA)) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s' but not '%s'", entitlementNameB, entitlementNameA));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown, or '%s' shown", entitlementNameB, entitlementNameA));
        }

        softAssertAll();
    }

    /**
     * Verify Game Library shows game X but not game Y
     *
     * @param driver Selenium WebDriver
     * @param gameX  (@link EntitlementInfo) game X
     * @param gameY  (@link EntitlementInfo) game Y
     * @return true if Game Library shows game X (but not game Y), false
     * otherwise
     */
    private boolean verifyLibraryGamesXNotY(WebDriver driver, EntitlementInfo gameX, EntitlementInfo gameY) {
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameTile gameTileX = new GameTile(driver, gameX.getOfferId());
        GameTile gameTileY = new GameTile(driver, gameY.getOfferId());
        boolean foundGameX = Waits.pollingWait(gameTileX::isGameTileVisible);
        boolean foundGameY = Waits.pollingWait(gameTileY::isGameTileVisible, 2000, 1000, 0);
        boolean result = foundGameX && !foundGameY;
        if (!result) {
            // Log to assist in debugging
            String resultX = foundGameX ? "found" : "not found";
            String resultY = foundGameY ? "found" : "not found";
            _log.warn(String.format("verifyLibraryGamesXNotY failed: X - '%s' %s, Y - '%s' %s", gameX.getName(), resultX, gameY.getName(), resultY));
        }
        return result;
    }
}
