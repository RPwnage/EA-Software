package com.ea.originx.automation.scripts.requiredtoplay;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.ActivationErrorDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.ActivationRequiredDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemCompleteDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for Ready-To-Play Un-entitled Activate:<br>
 * (1) Launch a game outside origin. Login to an un-entitled user<br>
 * (2) Verify that the 'Activation Required' dialog is displayed<br>
 * (3) Verify that redeeming a different game results in the 'Activation Error'
 * dialog<br>
 * (4) Verify that redeeming the un-entitled game results in game launch
 *
 * @author palui
 */
public class OAUnentitledActivate extends EAXVxTestTemplate {

    @TestRail(caseId = 11336)
    @Test(groups = {"requiredtoplay", "client_only", "full_regression"})
    public void testUnentitledActivate(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementProductCode = entitlement.getProductCode();

        final String otherProductCode = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE).getProductCode();

        UserAccount entitledUser = AccountManager.getEntitledUserAccount(entitlement);
        final String entitledUsername = entitledUser.getUsername();
        UserAccount unentitledUser = AccountManager.getInstance().createNewThrowAwayAccount(); // Default DOB 1986-01-01
        final String unentitledUsername = unentitledUser.getUsername();

        logFlowPoint("Launch Origin and login to entitled user: " + entitledUser.getUsername()); //1
        logFlowPoint("Navigate to game library and locate DiP Small game tile"); //2
        logFlowPoint("Download DiP Small game"); // 3
        logFlowPoint("Logout of Origin"); //4
        logFlowPoint("Launch DiP Small and login to un-entitled user: " + unentitledUsername); //5
        logFlowPoint("Verify DiP Small does not launch automatically"); //6
        logFlowPoint("Verify 'Activation Required' dialog appears stating the account does not own the entitlement"); //7
        logFlowPoint("Click 'Activate Game' and redeem DiP Large game"); //8
        logFlowPoint("Verify 'Game Activation Error' dialog appears. Close the dialog"); //9
        logFlowPoint("Verify 'Redeem' dialog also re-appears and redeem DiP Small game"); //10
        logFlowPoint("Verify Dip Small launches automatically"); //11

        //1
        // Use single instance client otherwise a new Origin instance starts when launching a game outside of Origin
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, entitledUser), true);

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.waitForDownloadable(), false);

        //3
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        logPassFail(downloaded, false);

        //4
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        logPassFail(loginPage.isOnLoginPage(), true);

        //5
        entitlement.launchOutsideOrigin(client);
        sleep(5000); // pause for relogin to refresh due to external game launch
        logPassFail(MacroLogin.startLogin(driver, unentitledUser), true);

        //6
        logPassFail(!entitlement.isLaunched(client), true);

        //7
        ActivationRequiredDialog activationRequiredDialog = new ActivationRequiredDialog(driver);
        boolean isActivationRequiredDialogOpen = activationRequiredDialog.verifyActivationRequiredDialogTitle();
        boolean isUnentitledMessagePresent = activationRequiredDialog.verifyActivationRequiredDialogMessage(entitlementName);
        logPassFail(isActivationRequiredDialogOpen && isUnentitledMessagePresent, false);

        //8
        activationRequiredDialog.clickActivateGameButton();
        logPassFail(MacroRedeem.redeemProductCode(driver, otherProductCode, entitlementName), false);

        //9
        RedeemCompleteDialog redeemCompleteDialog = new RedeemCompleteDialog(driver);
        redeemCompleteDialog.clickLaunchGameButton();
        ActivationErrorDialog errorDialog = new ActivationErrorDialog(driver);
        logPassFail(errorDialog.isOpen(), true);

        //10
        logPassFail(MacroRedeem.redeemProductCode(driver, entitlementProductCode, entitlementName), false);

        //11
        redeemCompleteDialog.clickLaunchGameButton();
        logPassFail(entitlement.isLaunched(client), true);

        entitlement.killLaunchedProcess(client);

        softAssertAll();
    }
}
