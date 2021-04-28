package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.FilterMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the purchased games appear under the "Purchased Games" filter in the Game Library
 * for a subscribed user
 *
 * @author svaghayenegar
 */
public class OAFilterPurchasedVaultGame extends EAXVxTestTemplate {

    @TestRail(caseId = 3064008)
    @Test(groups = {"originaccess", "full_regression"})
    public void OAFilterPurchasedVaultGame(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);

        logFlowPoint("Create an account and log in to Origin."); // 1
        logFlowPoint("Purchase an Origin Access subscription."); // 2
        logFlowPoint("Go to the GDP of a game that is in the vault, purchase it, and verify completion of payment"); // 3
        logFlowPoint("Go to 'Game Library', and verify it reached the page"); // 4
        logFlowPoint("Click on the Filters link in the top right corner, and observe the listed games under 'Purchased Games', and verify the game is under filter"); // 5

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        // 3
        logPassFail(MacroPurchase.purchaseEntitlement(driver, entitlement), true);

        // 4
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 5
        gameLibrary.openFilterMenu();
        FilterMenu filterMenu = new FilterMenu(driver);
        filterMenu.setPurchasedGames();
        sleep(2000);
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(entitlement.getOfferId()), true);
    }
}
