package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test a basic vault entitlement can be added to library through origin
 * premier subscription
 *
 * @author cdeaconu
 */
public class OABasicVaultAddEntitlement extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3486255)
    @Test(groups = {"gdp", "full_regression"})
    public void testBasicVaultGameEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);
        final String entitlementOfferId = entitlement.getOfferId();
        
        logFlowPoint("Launch Origin and login as newly registered account."); // 1
        logFlowPoint("Pruchase 'Origin Premier' subscription."); // 2
        logFlowPoint("Navigate to GDP of a game that has an edition in the 'Basic Vault' and is not owned."); // 3
        logFlowPoint("Verify 'Add to Library' primary CTA is showing."); // 4
        logFlowPoint("Click on 'Add to Library' CTA and verify the 'Added to your library' modal appears."); // 5
        logFlowPoint("Close the modal and verify 'View in Library' primary CTA is showing."); // 6
        logFlowPoint("Click on the 'View in Library' CTA and verify the page redirects to the 'Game Library' and opens the OGD of the game."); // 7
        logFlowPoint("Close the OGD and verify the game is now in the 'Game Library'."); // 8
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyAddToLibraryVaultGameCTAVisible(), true);
        
        // 5
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        
        // 6
        checkoutConfirmation.clickCloseCircle();
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAPresent(), true);
        
        // 7
        gdpActionCTA.clickViewInLibraryCTA();
        GameSlideout gameSlideout = new GameSlideout(driver);
        logPassFail(gameSlideout.waitForSlideout(), true);
        
        // 8
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(new GameLibrary(driver).isGameTileVisibleByOfferID(entitlementOfferId), true);
        
        softAssertAll();
    }
}