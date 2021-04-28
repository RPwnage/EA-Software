package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests options from the 'Game Library' menu
 * @author mdobre
 */
public class OAAddGameLibrarySubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 2726301)
    @Test(groups = {"gamelibrary", "full_regression"})
    public void testAddGameLibrarySubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        logFlowPoint("Launch Origin and login as newly registered user and purchase 'Origin Access'."); // 1
        logFlowPoint("Navigate to the 'Game Library'."); // 2
        logFlowPoint("Select 'Add a Game' from the 'Game Library' Menu."); // 3
        logFlowPoint("Verify there is an option to 'Add a Vault Game'."); // 4
        logFlowPoint("Verify there is an option to 'Redeem a Product Code'."); // 5
        logFlowPoint("Verify there is an option to 'Add a Non-Origin Game'."); // 6
        logFlowPoint("Click on Origin Access Vault Game and verify the user is brought to the 'Vault Page'."); // 7
        logFlowPoint("Add a vault game to the library, navigate to the game library and verify the entitlement is on the page."); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(Waits.pollingWait(() -> gameLibrary.verifyGameLibraryPageReached()), true);

        // 3
        gameLibrary.clickAddGame();
        logPassFail(gameLibrary.verifyAddGameButtonClicked(), true);

        // 4
        logPassFail(gameLibrary.verifyAddVaultGameOptionVisible(), true);

        // 5
        logPassFail(gameLibrary.verifyRedeemProductCodeOptionVisible(), true);

        // 6
        logPassFail(gameLibrary.verifyAddNonOriginGameOptionVisible(), true);

        // 7
        gameLibrary.clickAddVaultGameOption();
        logPassFail(new VaultPage(driver).isPageLoaded(), true);

        // 8
        MacroOriginAccess.addEntitlementAndHandleDialogs(driver, entitlement.getOfferId());
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(entitlement.getOfferId()), true);

        softAssertAll();
    }
}