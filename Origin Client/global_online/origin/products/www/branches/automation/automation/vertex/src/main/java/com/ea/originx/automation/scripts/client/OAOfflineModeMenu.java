package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests Menu in Offline mode
 *
 * @author mkalaivanan
 */
public class OAOfflineModeMenu extends EAXVxTestTemplate {

    @TestRail(caseId = 39454)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testOfflineModeMenu(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch Origin and login as user: " + userAccount.getUsername()); //1
        logFlowPoint("Select 'Go Offline' from the Origin Menu"); //2
        logFlowPoint("Verify 'Reload Page' is disabled from Origin Menu"); //3
        logFlowPoint("Verify 'Redeem Product Code','Add from the Vault' and 'Reload Game Library' are disabled in Games Menu"); //4
        logFlowPoint("Verify 'Add Friend' and 'Set Status' are disabled in the Friends Menu"); //5
        logFlowPoint("Verify 'My Profile' is disabled in the View Menu"); //6
        logFlowPoint("Verify 'Origin Help' and 'Origin Forum' are disabled in Help Menu"); //7
        logFlowPoint("Verify 'Shortcuts' is disabled in Help Menu");//8

        // 1
        WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = initPage(driver, context, LoginPage.class);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        boolean mainMenuOffline = mainMenu.verifyItemEnabledInOrigin("Go Online");
        if (mainMenuOffline) {
            logPass("Switch to offline mode successful");
        } else {
            logFailExit("Could not switch to Offline mode");
        }

        //3
        boolean verifyReloadPageDisabled = !mainMenu.verifyItemEnabledInOrigin("Reload Page");
        if (verifyReloadPageDisabled) {
            logPass("'Reload Page' is disabled in the Origin Menu");
        } else {
            logFail("'Reload Page' is not disabled in the Origin Menu");
        }

        //4
        boolean redeemCodeOptionDisabled = !mainMenu.verifyItemEnabledInGames("Redeem Product Code…");
        boolean addGameVaultOptionDisabled = !mainMenu.verifyItemExistsInGames("Add from the Vault");
        boolean reloadGamesOptionDisabled = !mainMenu.verifyItemEnabledInGames("Reload Game Library");
        if (redeemCodeOptionDisabled && addGameVaultOptionDisabled && reloadGamesOptionDisabled) {
            logPass("Verified 'Redeem Product Code…' and 'Reload Game Library' options are disabled and 'Add from the vault' is not present in Games menu");
        } else {
            logFail("Failed: 'Redeem Product Code…' and 'Reload Game Library' options are not disabled or 'Add from the vault' exists in Games Menu");
        }

        //5
        boolean addFriendOptionDisabled = !mainMenu.verifyItemEnabledInFriends("Add a Friend…");
        boolean setStatusOptionDisabled = !mainMenu.verifyItemEnabledInFriends("Set Status");
        if (addFriendOptionDisabled && setStatusOptionDisabled) {
            logPass("Verified 'Add a Friend' and 'Set Status' options are disabled in Friends Menu");
        } else {
            logFail("Failed: 'Add a friend' and 'Set Status' options are not disabled in Friends Menu");
        }

        //6
        boolean myProfileOptionDisabled = !mainMenu.verifyItemEnabledInView("My Profile");
        if (myProfileOptionDisabled) {
            logPass("Verified 'My Profile' is disabled in View Menu");
        } else {
            logFail("Failed: 'My Profile' is not disabled in View Menu");
        }

        //7
        boolean originHelpOptionDisabled = !mainMenu.verifyItemEnabledInHelp("Origin Help");
        boolean originForumOptionDisabled = !mainMenu.verifyItemEnabledInHelp("Origin Forum");
        if (originHelpOptionDisabled && originForumOptionDisabled) {
            logPass("Verified 'Origin Help' and 'Origin Forum' options are disabled in Help Menu");
        } else {
            logFail("Failed: 'Origin Help' and 'Origin Forum' options are not disabled in Help Menu");
        }

        //8
        if (mainMenu.verifyItemEnabledInHelp("Shortcuts")) {
            logPass("Verified 'Shortcuts' is visible on the 'Help' dropdown");
        } else {
            logFail("Failed: 'Shortcuts' option is not available on the 'Help' dropdown");
        }
        softAssertAll();
    }

}
