package com.ea.originx.automation.scripts.requiredtoplay;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.OfflineFlyout;
import com.ea.originx.automation.lib.pageobjects.dialog.ActivationOfflineErrorDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.ActivationRequiredDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for Required to Play in offline-mode : Make sure that user in offline
 * mode gets Not-Connected error message when trying to activate entitlement
 * which user does not own
 *
 * @author rchoi
 */
public class OAOfflineRTP extends EAXVxTestTemplate {

    @TestRail(caseId = 11337)
    @Test(groups = {"requiredtoplay", "client_only", "full_regression"})
    public void testOfflineRTP(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount entitledUser = AccountManager.getEntitledUserAccount(entitlement);
        String entitledUsername = entitledUser.getUsername();

        UserAccount unentitledUser = AccountManager.getUnentitledUserAccount();
        String unentitledUsername = unentitledUser.getUsername();

        logFlowPoint("Log into Origin with entitled User: " + entitledUsername); //1
        logFlowPoint("Navigate to Game Library and locate Dip small game tile"); //2
        logFlowPoint("Fully Download DiP Small"); //3
        logFlowPoint("Log out of Origin"); //4
        logFlowPoint("Launch DiP Small using its executable"); //5
        logFlowPoint("Log into Origin with the unentitled User: " + unentitledUsername); //6
        logFlowPoint("Verify dip small is not running"); //7
        logFlowPoint("Put Origin into Offline mode"); //8
        logFlowPoint("Verify Activation Required Dialog is present"); //9
        logFlowPoint("Verify user receives a 'Not Connected' error dialog by Selecting 'Activate Game'"); //10

        //1
        // use single instance of origin client. Otherwise new instance starts when launching a game outside of origin client.
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, entitledUser)) {
            logPass("Verified entitled User successfully logged into Origin with account " + entitledUsername);
        } else {
            logFailExit("Entitled User could not log into client with account  " + entitledUsername);
        }

        //2
        sleep(2000); // wait for page stabilization after login
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile entitlementTile = new GameTile(driver, entitlementOfferId);
        boolean locatedToEntitlement = entitlementTile.waitForDownloadable();
        if (locatedToEntitlement) {
            logPass("Verified entitled User navigated to Game Library page with DiP Small entitlement tile");
        } else {
            logFailExit("Failed: Entitled User could not navigate to Game Library or DiP Small entitlement tile is not available");
        }

        //3
        boolean isDownloadedInstalled = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId);
        if (isDownloadedInstalled) {
            logPass("Verified entitled User successfully downloaded and installed DiP Small entitlement.");
        } else {
            logFailExit("Failed: Entitled User could not downloaded and installed DiP small entitlement.");
        }

        //4
        MainMenu mainMenu = new MainMenu(driver);
        LoginPage loginPage = mainMenu.selectLogOut();
        sleep(2000); // pause for Login page to appear
        boolean isLoginPage = loginPage.getCurrentURL().matches(OriginClientData.LOGIN_PAGE_URL_REGEX);
        if (isLoginPage) {
            logPass("Verified entitled User successfully logged out");
        } else {
            logFailExit("Failed: Entitled User could NOT log out");
        }

        //5
        if (entitlement.launchOutsideOrigin(client)) {
            logPass("Verified Launched DiP Small successful ");
        } else {
            logFailExit("Failed: Could not launch DiP Small");
        }

        //6
        sleep(5000); // wait for login page
        Waits.waitForPageThatStartsWith(driver, "https", 60);
        if (MacroLogin.startLogin(driver, unentitledUser)) {
            logPass("Verified unentitled User successfully logged into Origin with account " + unentitledUsername);
        } else {
            logFailExit("Failed: Unentitled User could not log into client with account  " + unentitledUsername);
        }

        //7
        sleep(2000); // wait for page stabilization
        if (!entitlement.isLaunched(client)) {
            logPass("Verified DiP small is NOT running");
        } else {
            logFailExit("Failed: DiP small is still running");
        }

        //8
        mainMenu.selectGoOffline(false);
        Thread.sleep(1000); // wait for page stabilization
        if (mainMenu.verifyItemEnabledInOrigin("Go Online")) {
            logPass("Verified origin is in offline mode");
        } else {
            logFailExit("Failed: Origin is NOT in offline mode");
        }

        //9
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        new OfflineFlyout(driver).closeOfflineFlyout(); // close 'Offline flyout' which may interfere with game tiles operations
        ActivationRequiredDialog activationRequiredDialog = new ActivationRequiredDialog(driver);
        boolean activationRequiredDialogShown = activationRequiredDialog.verifyActivationRequiredDialogTitle();
        if (activationRequiredDialogShown) {
            logPass("Verified Activation Required Dialog is present");
        } else {
            logFailExit("Failed: Activation Required Dialog is NOT present");
        }

        //10
        activationRequiredDialog.clickActivateGameButton();
        sleep(2000); // wait for page stabilization
        boolean errorDialogShown = new ActivationOfflineErrorDialog(driver).isOpen();
        if (errorDialogShown) {
            logPass("Verified 'Not Connected' Error dialog is present");
        } else {
            logFailExit("Failed: 'Not Connected' Error dialog is NOT present");
        }
        softAssertAll();
    }

}
