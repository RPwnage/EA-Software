package com.ea.originx.automation.scripts.gdp;

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
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase entitlement through vault
 *
 * @author cdeaconu
 */
public class OAPurchaseEntitlementThroughVault extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3002000)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseEntitlementThroughVault(ITestContext context) throws Exception {
         
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo vaultSingleEditionEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);
        final EntitlementInfo vaultMultipleEditionEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        final String vaultSingleEditionEntitlementName = vaultSingleEditionEntitlement.getName();
        final String vaultMultipleEditionEntitlementOfferId = vaultMultipleEditionEntitlement.getOfferId();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase Origin Access."); // 2
        logFlowPoint("Navigate to GDP of a single edition vault entitlement."); // 3
        logFlowPoint("Click on the 'Add to Library' CTA and verify the 'Added to your library' modal appears"); // 4
        logFlowPoint("Close the modal and verify the primary button changes to 'View in Library'."); // 5
        logFlowPoint("Click on the 'View in Library' primary button and verify the page redirects to 'Game Library'."); // 6
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 7
        logFlowPoint("Navigate to GDP of a multiple edition vault entitlement."); // 8
        logFlowPoint("Click on the 'Add to Library' CTA and verify the 'Added to your library' modal appears"); // 9
        logFlowPoint("Close the modal and verify the primary button changes to 'View in Library'."); // 10
        logFlowPoint("Click on the 'View in Library' primary button and verify the page redirects to 'Game Library'."); // 11
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 12
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, vaultSingleEditionEntitlement), true);
        
        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        logPassFail(checkoutConfirmation.waitIsVisible(), true);
        
        // 5
        checkoutConfirmation.clickCloseCircle();
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);
        
        // 6
        gdpActionCTA.clickViewInLibraryCTA();
        GameLibrary gameLibrary = new GameLibrary(driver);
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 7
        logPassFail(gameLibrary.isGameTileVisibleByName(vaultSingleEditionEntitlementName), true);
        
        // 8
        logPassFail(MacroGDP.loadGdpPage(driver, vaultMultipleEditionEntitlement), true);
        
        // 9
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        logPassFail(checkoutConfirmation.isDialogVisible(), true);
        
        // 10
        checkoutConfirmation.clickCloseCircle();
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);
        
        // 11
        gdpActionCTA.clickViewInLibraryCTA();
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 12
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(vaultMultipleEditionEntitlementOfferId), true);
        
        softAssertAll();
    }
}