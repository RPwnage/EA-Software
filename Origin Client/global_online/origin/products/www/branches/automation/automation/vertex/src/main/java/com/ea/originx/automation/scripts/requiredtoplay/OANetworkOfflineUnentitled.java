package com.ea.originx.automation.scripts.requiredtoplay;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNetworkOffline;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.ActivationOfflineErrorDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.ActivationRequiredDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
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
 * Test launching an un-entitled game with network offline login
 *
 * @author palui
 */
public class OANetworkOfflineUnentitled extends EAXVxTestTemplate {

    @TestRail(caseId = 11388)
    @Test(groups = {"requiredtoplay", "client_only", "full_regression"})
    public void testNetworkOfflineUnentitled(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        String entitlementName = entitlement.getName();
        String entitlementOfferId = entitlement.getOfferId();

        UserAccount entitledUser = AccountManager.getEntitledUserAccount(entitlement);
        String entitledUsername = entitledUser.getUsername();

        UserAccount unentitledUser = AccountManager.getUnentitledUserAccount();
        String unentitledUsername = unentitledUser.getUsername();

        logFlowPoint(String.format("Launch Origin and login as entitled user: %s", entitledUsername)); //1
        logFlowPoint(String.format("Navigate to game library and locate '%s' game tile", entitlementName)); //2
        logFlowPoint(String.format("Download '%s' game", entitlementName)); //3
        logFlowPoint(String.format("Logout from entitled user: %s", entitledUsername)); //4
        logFlowPoint(String.format("Login as un-entitled user: %s", unentitledUsername)); //5
        logFlowPoint("Disconnect Origin from the network"); //6
        logFlowPoint(String.format("Logout from un-entitled user: %s and launch '%s' from the executable", unentitledUsername, entitlementName)); //7
        logFlowPoint(String.format("Login again as un-entitled user: %s", unentitledUsername)); //8
        logFlowPoint("Verify a 'Activation Require' dialog appears stating the account does not own the entitlement"); //9
        logFlowPoint("Verify Origin is disconnected from the network"); //10
        logFlowPoint("Click 'Activate Game' at the 'Activation Require' dialog. Verify 'Not Connected' Error dialog appears"); //11

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, entitledUser)) {
            logPass(String.format("Verified login successful as entitled user: %s", entitledUsername));
        } else {
            logFailExit(String.format("Failed: Cannot login as entitled user: %s", entitledUsername));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with '%s' game tile", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or locate'%s' game tile", entitlementName));
        }

        //3
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId)) {
            logPass(String.format("Verified successful download of '%s' game", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot download '%s' game", entitlementName));
        }

        //4
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage()) {
            logPass("Verified logout of entitled user successful");
        } else {
            logFailExit("Failed: Cannot logout of entitled user");
        }

        //5
        if (MacroLogin.startLogin(driver, unentitledUser)) {
            logPass("Verified login successful as un-entitled user: " + unentitledUsername);
        } else {
            logFailExit("Failed: Cannot login as un-entitled user: " + unentitledUsername);
        }

        //6
        if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
            logPass("Verified Origin is disconnected from the Network");
        } else {
            logFailExit("Failed: Origin is not disconnected from the Network");
        }

        // Logging out too soon from the client sends you to an error page on the
        // the login screen. We'll sleep a bit here before continuing
        // See ORPLAT-8262
        sleep(15000);

        //7
        loginPage = new MainMenu(driver).selectLogOut();
        if (loginPage.isOnLoginPage()) {
            logPass("Verified logout of un-entitled user successful");
        } else {
            logFailExit("Failed: Cannot logout of un-enetiled user");
        }

        //8
        entitlement.launchOutsideOrigin(client);
        sleep(5000); // pause for login window to refresh due to external game launch
        if (MacroLogin.networkOfflineClientLogin(driver, unentitledUser)) {
            logPass("Verified login again successful as un-entitled user: " + unentitledUsername);
        } else {
            logFailExit("Failed: Cannot login again as un-entitled user: " + unentitledUsername);
        }

        //9
        ActivationRequiredDialog arDialog = new ActivationRequiredDialog(driver);
        // Can take couple minutes for the AR dialog to show
        boolean arDialogOpened = Waits.pollingWait(arDialog::verifyActivationRequiredDialogTitle, 180000, 1000, 15000);
        boolean unentitledMessagePresent = arDialog.verifyActivationRequiredDialogMessage(entitlementName);
        if (arDialogOpened && unentitledMessagePresent) {
            logPass("Verified 'Activation Required' dialog appears stating that the account does not own the entitlement");
        } else {
            logFailExit("Failed: 'Activation Required' dialog does not appear or does not report the unentitlement");
        }

        //10
        // Our simulated offline using firewall means Origin does not recognize offline after login
        // even though it is blocked by the firewall.
        // Re-enter network offline to make Origin recognize it is offline
        if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
            logPass("Verified Origin is still disconnected from the network");
        } else {
            logFailExit("Failed: Origin is not still disconnected from the network");
        }

        //11
        arDialog.clickActivateGameButton();
        ActivationOfflineErrorDialog aoeDialog = new ActivationOfflineErrorDialog(driver);
        boolean errorDialogShown = Waits.pollingWait(aoeDialog::isOpen);
        boolean errorDialogMessageExpected = aoeDialog.verifyMessage();
        if (errorDialogShown && errorDialogMessageExpected) {
            logPass("Verified 'Not Connected' Error dialog appears with the expected title and message");
        } else {
            logFailExit("Failed: 'Not Connected' Error dialog does not appear or does not have the expected title or message");
        }

        softAssertAll();
    }
}
