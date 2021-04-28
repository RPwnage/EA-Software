package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests purchasing a single edition premier vault entitlement with a basic
 * origin access subscription redirect the user to the 'Game library' and opens
 * the OGD of the game
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionPremierVaultBasicSubscriber extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3548527)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseSingleEditionPremierVaultBasicSubscriber(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String userEmail = userAccount.getEmail();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT_2);
        final String offerId = entitlement.getOfferId();
        final String environment = OSInfo.getEnvironment();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase 'Origin Basic Access'."); // 2
        logFlowPoint("Navigate to GDP of a single edition premier vault entitlement."); // 3
        logFlowPoint("Verify 'Get the Game' primary CTA is visible."); // 4
        logFlowPoint("Click 'Get the Game' CTA and verify the page redirects to the 'Access Interstitial Page'."); // 5
        logFlowPoint("Click on the 'Buy' CTA on the 'Origin Access Interstitial Page' and verify the checkout flow starts"); // 6
        logFlowPoint("Complete the checkout flow and close the 'Thank you' modal."); // 7
        logFlowPoint("Verify 'View in Library' primary CTA is showing."); // 8
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library' and opens the OGD of the game."); // 9
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 10
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        if(environment.equals("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOABasicSubscription(driver, userEmail), true);
        } else {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }

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
        accessInterstitialPage.clickBuyGameCTA();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);

        // 7
        if(environment.equals("production")) {
            logPassFail(MacroPurchase.handlePaymentInfoPageForProdPurchases(driver, entitlement.getName(), userEmail), true);
        } else {
            logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        }

        // 8
        logPassFail(gdpActionCTA.verifyViewInLibraryCTAVisible(), true);
        
        // 9
        gdpActionCTA.clickViewInLibraryCTA();
        GameSlideout gameSlideout = new GameSlideout(driver);
        logPassFail(gameSlideout.waitForSlideout(), true);
        
        // 10
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(new GameLibrary(driver).isGameTileVisibleByOfferID(offerId), true);
        
        softAssertAll();
    }
}