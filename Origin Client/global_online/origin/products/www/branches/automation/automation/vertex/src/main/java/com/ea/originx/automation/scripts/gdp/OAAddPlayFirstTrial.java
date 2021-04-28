package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.TryTheGameOutDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Test adding a 'Play first trial' Entitlement to 'Game Library'
 *
 * @author cdeaconu
 */
public class OAAddPlayFirstTrial extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064067)
    @Test(groups = {"gdp", "full_regression"})
    public void testAddPlayFirstTrial(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT_2);
        final String entitlementId = entitlement.getOfferId();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase Origin Access."); // 2
        logFlowPoint("Navigate to GDP of a game that has 'Play First Trial' available."); // 3
        logFlowPoint("Verify the 'View Trial' secondary button is showing below the primary button."); // 4
        logFlowPoint("Click on the 'View Trial' CTA and verify a demo/trial modal appears."); // 5
        logFlowPoint("Click on the 'Add to Game Library' and verify an 'Added to your library' message appears."); // 6
        logFlowPoint("Navigate to 'Game Library' and verify the game appears."); // 7
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyViewTrialCTAVisible(), true);
        
        // 5
        gdpActionCTA.clickViewTrialCTA();
        TryTheGameOutDialog tryTheGameOutDialog = new TryTheGameOutDialog(driver);
        logPassFail(tryTheGameOutDialog.waitIsVisible(), true);
       
        // 6
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        tryTheGameOutDialog.clickAddGameToLibraryCTA();
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        
        // The 'Checkout' dialog remains opened after the
        // 'Add game to library' CTA is clicked
        checkoutConfirmation.closeAndWait();
        
        // 7
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(entitlementId), true);
        
        softAssertAll();
    }
}