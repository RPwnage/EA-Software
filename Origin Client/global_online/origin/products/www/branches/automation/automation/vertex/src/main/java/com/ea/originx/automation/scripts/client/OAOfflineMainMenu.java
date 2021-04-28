package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNetworkOffline;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests the functionality of the main menu when Origin is offline.<br><br>
 * <p>
 * This is the base test script, which is called by the following parameterized
 * test scripts: <br>
 * - OANetworkOfflineMainMenu<br>
 * - OAOfflineModeMainMenu<br>
 *
 * @author lscholte
 */
public abstract class OAOfflineMainMenu extends EAXVxTestTemplate {

    /**
     * The main test function which all parameterized test cases call.
     *
     * @param context        The context we are using
     * @param useOfflineMode True if the client should use the offline mode
     *                       feature. False if the client should be disconnected from the network
     * @param testName       The name of the test. We need to pass this up so
     *                       initLogger properly attaches to the CRS test case.
     * @throws Exception
     */
    public void testOfflineMainMenu(ITestContext context, boolean useOfflineMode, String testName) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String reloadPage = "Reload Page";
        final String redeemProductCode = "Redeem Product Code\u2026";
        final String reloadGameLibrary = "Reload Game Library";
        final String addFriend = "Add a Friend\u2026";
        final String setStatus = "Set Status";
        final String myProfile = "My Profile";
        final String originHelp = "Origin Help";
        final String originForum = "Origin Forum";

        UserAccount user = AccountManager.getRandomAccount();

        logFlowPoint(String.format("Launch Origin and login as user %s", user.getUsername())); //1
        if (useOfflineMode) {
            logFlowPoint("Select 'Go Offline' from the Origin menu"); //2a
        } else {
            logFlowPoint("Disconnect Origin from the network"); //2b
        }
        logFlowPoint(String.format("Verify '%s' is disabled in the origin menu", reloadPage)); //3
        logFlowPoint(String.format("Verify '%s' is disabled in the Games menu", redeemProductCode)); //4
        logFlowPoint(String.format("Verify '%s' is disabled in the Games menu", reloadGameLibrary)); //5
        logFlowPoint(String.format("Verify '%s' is disabled in the Friends menu", addFriend)); //6
        logFlowPoint(String.format("Verify '%s' is disabled in the Friends menu", setStatus)); //7
        logFlowPoint(String.format("Verify '%s' is disabled in the View menu", myProfile)); //8
        logFlowPoint(String.format("Verify '%s' is disabled in the Help menu", originHelp)); //9
        logFlowPoint(String.format("Verify '%s' is disabled in the Help menu", originForum)); //10

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");
        }

        MainMenu mainMenu = new MainMenu(driver);
        if (useOfflineMode) {

            //2a
            mainMenu.selectGoOffline();
            if (mainMenu.verifyItemEnabledInOrigin("Go Online")) {
                logPass("Successfully switched Origin to offline mode");
            } else {
                logFailExit("Failed to switch Origin to offline mode");
            }
        } else //2b
            if (MacroNetworkOffline.enterOriginNetworkOffline(driver)) {
                logPass("Successfully disconnected Origin from the network");
            } else {
                logFailExit("Failed to disconnect Origin from the network");
            }

        //3
        if (!mainMenu.verifyItemEnabledInOrigin(reloadPage)) {
            logPass(String.format("'%s' is disabled in the Origin menu", reloadPage));
        } else {
            logFail(String.format("'%s' is not disabled in the Origin menu", reloadPage));
        }

        //4
        if (!mainMenu.verifyItemEnabledInGames(redeemProductCode)) {
            logPass(String.format("'%s' is disabled in the Games menu", redeemProductCode));
        } else {
            logFail(String.format("'%s' is not disabled in the Games menu", redeemProductCode));
        }

        //5
        if (!mainMenu.verifyItemEnabledInGames(reloadGameLibrary)) {
            logPass(String.format("'%s' is disabled in the Games menu", reloadGameLibrary));
        } else {
            logFail(String.format("'%s' is not disabled in the Games menu", reloadGameLibrary));
        }

        //6
        if (!mainMenu.verifyItemEnabledInFriends(addFriend)) {
            logPass(String.format("'%s' is disabled in the Friends menu", addFriend));
        } else {
            logFail(String.format("'%s' is not disabled in the Friends menu", addFriend));
        }

        //7
        if (!mainMenu.verifyItemEnabledInFriends(setStatus)) {
            logPass(String.format("'%s' is disabled in the Friends menu", setStatus));
        } else {
            logFail(String.format("'%s' is not disabled in the Friends menu", setStatus));
        }

        //8
        if (!mainMenu.verifyItemEnabledInView(myProfile)) {
            logPass(String.format("'%s' is disabled in the View menu", myProfile));
        } else {
            logFail(String.format("'%s' is not disabled in the View menu", myProfile));
        }

        //9
        if (!mainMenu.verifyItemEnabledInHelp(originHelp)) {
            logPass(String.format("'%s' is disabled in the Help menu", originHelp));
        } else {
            logFail(String.format("'%s' is not disabled in the Help menu", originHelp));
        }

        //10
        if (!mainMenu.verifyItemEnabledInHelp(originForum)) {
            logPass(String.format("'%s' is disabled in the Help menu", originForum));
        } else {
            logFail(String.format("'%s' is not disabled in the Help menu", originForum));
        }

        softAssertAll();
    }

}
