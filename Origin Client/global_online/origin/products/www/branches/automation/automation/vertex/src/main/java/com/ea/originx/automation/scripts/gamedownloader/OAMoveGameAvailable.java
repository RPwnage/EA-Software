package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideoutCogwheel;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Move Game' option is available in game tile context menu/OGD game setting menu, if the game is installed
 * - OAMoveGameAvailableInGameTileContextMenu
 * - OAMoveGameAvailableInOGDGameSettingMenu
 * @author sunlee
 */
public class OAMoveGameAvailable extends EAXVxTestTemplate {

    public enum METHOD_TO_MOVE_GAME {USE_GAME_TILE_CONTEXT_MENU, USE_OGD_GAME_SETTINGS};

    /**
     * The main test function which all parameterized test case call.
     * @param context           The Context we are using
     * @param howToMoveGame     How to move game
     * @param testName          The name of the test. We need to pass this up so
     * private static final Logger _log = LogManager.getLogger(OAMoveGameAvailable.class);
     *
     * @throws Exception
     */
    public void testMoveGameAvailable(ITestContext context, METHOD_TO_MOVE_GAME howToMoveGame, String testName) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BEJEWELED3);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();
        logFlowPoint(String.format("Launch and login to Origin client with an account owns a DiP entitlement %s", entitlementName)); //1
        logFlowPoint("Navigate to game library"); //2
        logFlowPoint("Download and install the entitlement"); //3
        logFlowPoint("Verify the entitlement is playable"); //4
        logFlowPoint("Log out and log into Origin"); //5
        logFlowPoint("Navigate to game library"); //6
        if(howToMoveGame == METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            logFlowPoint("Right click on the entitlement and verify 'Move Game' option is available"); //7 - 1
        } else {
            logFlowPoint("Open OGD page of the entitlement"); //7 - 1
            logFlowPoint("Click game setting and verify 'Move Game' option is available"); //8
        }

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 3
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferId), true);

        // 4
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.isReadyToPlay(), true);

        // 5
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 6
        gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        if(howToMoveGame == METHOD_TO_MOVE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            // 7
            logPassFail(new GameTile(driver, entitlementOfferId).verifyMoveGameVisible(), true);
        } else {
            // 7 - 1
            GameSlideout entitlementSlideout = gameTile.openGameSlideout();
            logPassFail(entitlementSlideout.waitForSlideout(), true);

            //8
            GameSlideoutCogwheel gameSlideoutCogwheel = new GameSlideout(driver).getCogwheel();
            logPassFail(gameSlideoutCogwheel.verifyMoveGameVisible(), true);
        }

        softAssertAll();
    }
}
