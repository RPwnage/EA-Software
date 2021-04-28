package com.ea.originx.automation.scripts.feature.gamelibrary;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests resizing the game tiles
 *
 * @author nvarthakavi
 */
public class OAResizeGameTile extends EAXVxTestTemplate {

    @TestRail(caseId = 3087353)
    @Test(groups = {"gamelibrary", "gamelibrary_smoke", "client_only", "allfeaturesmoke"})
    public void testResizeGameTile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s'", username, entitlementName));//1
        logFlowPoint(String.format("Navigate to the Game Library and verify game '%s' appears in the game library", entitlementName)); //2
        logFlowPoint("Set the tiles to the medium size using the buttons at the top and verify the tiles are set to medium size");//3
        logFlowPoint("Set the tiles to the small size using the buttons at the top and verify the tiles are set to small size");//4
        logFlowPoint("Set the tiles to the large size using the buttons at the top and verify the tiles are set to large size");//5

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        gameLibrary.waitForPage();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        if (gameTile.isGameTileVisible()) {
            logPass(String.format("Verified successful navigation to Game Library with game '%s' in it",
                    entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or cannot locate game '%s'",
                    entitlementName));
        }

        //3
        gameLibrary.setToMediumTiles();
        if (gameLibrary.verifyTileSizeChangedToMedium()) {
            logPass("Verified successful the tiles are set to medium size");
        } else {
            logFail("Failed: The tiles are  not set to medium size");
        }

        //4
        gameLibrary.setToSmallTiles();
        if (gameLibrary.verifyTileSizeChangedToSmall()) {
            logPass("Verified successful the tiles are set to small size");
        } else {
            logFail("Failed: The tiles are  not set to small size");
        }

        //5
        gameLibrary.setToLargeTiles();
        if (gameLibrary.verifyTileSizeChangedToLarge()) {
            logPass("Verified successful the tiles are set to large size");
        } else {
            logFail("Failed: The tiles are  not set to large size");
        }

        softAssertAll();

    }
}
