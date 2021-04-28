package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideoutCogwheel;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests 'Locate Game' option is available in game tile context menu/OGD game setting menu, if the game is not installed
 * - OALocateGameAvailableInGameTileContextMenu
 * - OALocateGameAvailableInOGDGameSettingMenu
 * @author sunlee
 */
public class OALocateGameAvailable extends EAXVxTestTemplate {

    public enum METHOD_TO_LOCATE_GAME {USE_GAME_TILE_CONTEXT_MENU, USE_OGD_GAME_SETTINGS};

    /**
     * The main test function which all parameterized test case call.
     * @param context           The Context we are using
     * @param howToMoveGame     How to move game
     * @param testName          The name of the test. We need to pass this up so
     * private static final Logger _log = LogManager.getLogger(OAMoveGameAvailable.class);
     *
     * @throws Exception
     */
    public void testLocateGameAvailable(ITestContext context, METHOD_TO_LOCATE_GAME howToMoveGame, String testName) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BEJEWELED3);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint(String.format("Launch and login to Origin client with an account owns a DiP entitlement %s", entitlementName)); //1
        logFlowPoint("Navigate to game library"); //2
        if(howToMoveGame == METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            logFlowPoint("Right click on the entitlement and verify 'Locate Game' option is available"); //3 - 1
        } else {
            logFlowPoint("Open OGD page of the entitlement");
            logFlowPoint("Click game setting and verify 'Locate Game' option is available"); //7 - 2
        }

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        if(howToMoveGame == METHOD_TO_LOCATE_GAME.USE_GAME_TILE_CONTEXT_MENU) {
            // 3 - 1
            logPassFail(new GameTile(driver, entitlementOfferId).verifyLocateGameVisible(), true);
        } else {
            // 3 - 2
            GameSlideout entitlementSlideout = new GameTile(driver, entitlementOfferId).openGameSlideout();
            logPassFail(entitlementSlideout.waitForSlideout(), true);

            // 4
            GameSlideoutCogwheel gameSlideoutCogwheel = new GameSlideout(driver).getCogwheel();
            logPassFail(gameSlideoutCogwheel.verifyLocateGameVisible(), true);
        }

        softAssertAll();
    }
}
