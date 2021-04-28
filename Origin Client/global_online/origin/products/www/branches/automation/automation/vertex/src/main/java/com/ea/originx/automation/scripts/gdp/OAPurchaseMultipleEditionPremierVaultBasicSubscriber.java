package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
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
 * Tests purchasing a multiple edition premier vault entitlement with a basic
 * origin access subscription redirect the user to the 'Game library' and opens
 * the OGD of the game
 *
 * @author cdeaconu
 */
public class OAPurchaseMultipleEditionPremierVaultBasicSubscriber extends EAXVxTestTemplate {
    
    @TestRail(caseId = 3567995)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseMultipleEditionPremierVaultBasicSubscriber(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GDP_PREMIER_RELEASE_MULTIPLE_EDITION);
        final String offerId = entitlement.getOfferId();
        final String ocdPath = entitlement.getOcdPath();
        
        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Purchase 'Origin Basic Access'."); // 2
        logFlowPoint("Navigate to GDP of a multiple edition premier vault entitlement."); // 3
        logFlowPoint("Verify 'Get the Game' primary CTA is visible."); // 4
        logFlowPoint("Click 'Get the Game' CTA and verify the page redirects to the 'Access Interstitial Page'."); // 5
        logFlowPoint("Click on the 'Buy' CTA and verify the page redirects to OSP (Offer Selection Page)."); // 6
        logFlowPoint("Click on 'Buy <price text>' CTA for an edition on the comparison table and verify the checkout flow starts."); // 7
        logFlowPoint("Complete the checkout flow and close the 'Thank you' modal."); // 8
        logFlowPoint("Verify 'View in Library' primary CTA is showing."); // 9
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library' and opens the OGD of the game."); // 10
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 11
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyGetTheGameCTAVisible(), true);
        
        // 5
        gdpActionCTA.clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.waitForPageToLoad();
        logPassFail(accessInterstitialPage.verifyOSPInterstitialPageReached(), true);
        
        // 6
        accessInterstitialPage.clickBuyGameOSPCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.waitForPageToLoad();
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);
        
        // 7
        offerSelectionPage.clickPrimaryButton(ocdPath);
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);
        
        // 8
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        
        // 9
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);
        
        // 10
        gdpActionCTA.clickViewInLibraryCTA();
        GameSlideout gameSlideout = new GameSlideout(driver);
        logPassFail(gameSlideout.waitForSlideout(), true);
        
        // 11
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(new GameLibrary(driver).isGameTileVisibleByOfferID(offerId), true);
        
        softAssertAll();
    }
}