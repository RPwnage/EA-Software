package com.ea.originx.automation.scripts.feature.ogd;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Origin game details favorite and unfavorite
 *
 * @author palui
 */
public class OAOGDFavorites extends EAXVxTestTemplate {

    @TestRail(caseId = 3103323)
    @Test(groups = {"gamelibrary", "ogd", "ogd_smoke", "allfeaturesmoke"})
    public void testOGDFavorites(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        String entitlementName = entitlement.getName();
        String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, entitlementName));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify game '%s' is visible", entitlementName));//2
        logFlowPoint(String.format("Open 'Origin Game Details' slideout for '%s'", entitlementName));//3
        logFlowPoint(String.format("Verify 'Origin Game Details' slideout shows '%s' as un-favorited", entitlementName));//4
        logFlowPoint(String.format("Click the slideout's 'Favorite' button. Verify slideout now shows '%s' as favorited", entitlementName));//5
        logFlowPoint(String.format("Close the slideout. Verify '%s' has been added to the Game Library's favorites list", entitlementName));//6
        logFlowPoint(String.format("Re-open the slideout and click the 'Favorite' button. Verify slideout now shows '%s' as unfavorited", entitlementName));//7
        logFlowPoint(String.format("Close the slideout. Verify '%s' has been removed from the Game Library's favorites list", entitlementName));//8

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        gameLibrary.waitForPage();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        logPassFail(gameTile.isGameTileVisible(), true);
        // The game might have been favorited, remove from the favorites list
        if (gameLibrary.isFavoritesGameTile(entitlementOfferId)) {
            gameTile.removeFromFavorites();
        }

        //3
        gameTile.showGameDetails();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        logPassFail(Waits.pollingWait(() -> gameSlideout.verifyGameTitle(entitlementName)), true);

        //4
        logPassFail(Waits.pollingWait(() -> !gameSlideout.isFavorited()), true);

        //5
        gameSlideout.clickFavoritesButton();
        logPassFail(Waits.pollingWait(gameSlideout::isFavorited), true);

        //6
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(Waits.pollingWait(() -> gameLibrary.isFavoritesGameTile(entitlementOfferId)), true);

        //7
        gameTile.showGameDetails();
        gameSlideout.waitForSlideout();
        gameSlideout.clickFavoritesButton();
        logPassFail(Waits.pollingWait(() -> !gameSlideout.isFavorited()), true);

        //8
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(Waits.pollingWait(() -> !gameLibrary.isFavoritesGameTile(entitlementOfferId)), true);

        softAssertAll();
    }
}