package com.ea.originx.automation.scripts.originaccess.gamelibrary.vaultgames;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
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
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests adding game to vault as a subscriber
 *
 * @author svaghayenegar
 */
public class OAStandardSubscriberVaultGame extends EAXVxTestTemplate {

    @TestRail(caseId = 2997367)
    @Test(groups = {"originaccess", "full_regression"})
    public void OAStandardSubscriberVaultGame(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_17);

        logFlowPoint("Log into Origin as a new user"); // 1
        logFlowPoint("Purchase 'Origin Access'"); // 2
        logFlowPoint("Verify vault games page loads"); //3
        logFlowPoint("Click on any game listed in the vault, verify that it navigates to the GDP (Game Details Page) for the game"); //4
        logFlowPoint("Click on the 'Add to Library' CTA in the GDP, verify the game added to library message is shown"); //5
        logFlowPoint("Navigate to the game library, verify that the vault game added appears in the library"); //6

        WebDriver driver = startClientObject(context, client);

        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        // 3
        VaultPage vaultPage = new VaultPage(driver);
        logPassFail(vaultPage.verifyPageReached(), true);

        // 4
        vaultPage.clickEntitlementByOfferID(entitlement.getOfferId());
        GDPHeader gdpHeader = new GDPHeader(driver);
        logPassFail(gdpHeader.verifyGDPHeaderReached() && gdpHeader.verifyGameTitle(entitlement.getParentName()), true);

        // 5
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.isDialogVisible(), true);

        // 6
        checkoutConfirmation.close();
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(entitlement.getOfferId()), true);

        softAssertAll();
    }
}
