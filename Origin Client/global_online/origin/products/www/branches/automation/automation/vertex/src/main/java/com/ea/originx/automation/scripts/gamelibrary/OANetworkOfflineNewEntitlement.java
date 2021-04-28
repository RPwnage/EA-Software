package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNetworkOffline;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding a new game and then re-login while Origin client is disconnected
 * from the network and verifying the game is still in the game library
 *
 * @author palui
 */
public class OANetworkOfflineNewEntitlement extends EAXVxTestTemplate {

    @TestRail(caseId = 9713)
    @Test(groups = {"gamelibrary", "client_only", "full_regression"})
    public void testNetworkOfflineNewEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String offerId = entitlement.getOfferId();
        final String productCode = entitlement.getProductCode();

        UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as newly-registered user"); //1
        logFlowPoint(String.format("Redeem '%s' using its product code", entitlementName)); //2
        logFlowPoint(String.format("Navigate to game library and verify '%s' game tile exists", entitlementName)); //3
        logFlowPoint("Logout and exit from Origin"); //4
        logFlowPoint("Disconnect Origin from the network"); //5
        logFlowPoint("Launch Origin and verify Login Window is in offline mode"); //6
        logFlowPoint("Login again as user"); //7
        logFlowPoint(String.format("Navigate to game library and verify '%s' still appears", entitlementName)); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as newly registered user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as newly registered user %s", username));
        }

        //2
        new MainMenu(driver).selectRedeemProductCode();
        if (MacroRedeem.redeemProductCode(driver, productCode)) {
            logPass(String.format("Verified '%s' redeemed successfully", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot redeem '%s'", entitlementName));
        }

        //3
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go back to main window after redeem dialog
        new NavigationSidebar(driver).gotoGameLibrary();
        boolean foundGame = new GameTile(driver, offerId).isGameTileVisible();
        if (foundGame) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown", entitlementName));
        }

        //4
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage()) {
            logPass(String.format("Verified logout from user %s successful", username));
        } else {
            logFailExit(String.format("Failed: Cannot logout from user %s", username));
        }
        loginPage.close();

        //5
        if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
            logPass("Verified Origin is disconnected from the Network");
        } else {
            logFailExit("Failed: Origin is not disconnected from the Network");
        }

        //6
        driver = startClientObject(context, client);
        loginPage = new LoginPage(driver);
        if (loginPage.verifySiteStripeOfflineMessage()) {
            logPass("Verified Origin launches and Login Window is in offline mode");
        } else {
            logFailExit("Failed: Cannot launch Origin or Login window is not in offline mode");
        }

        //7
        if (MacroLogin.networkOfflineClientLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful (with Origin disconnected) as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login (with Origin disconnected) as user %s", username));
        }

        //8
        new NavigationSidebar(driver).gotoGameLibrary();
        foundGame = new GameTile(driver, offerId).isGameTileVisible();
        if (foundGame) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s'", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown", entitlementName));
        }

        softAssertAll();
    }
}
