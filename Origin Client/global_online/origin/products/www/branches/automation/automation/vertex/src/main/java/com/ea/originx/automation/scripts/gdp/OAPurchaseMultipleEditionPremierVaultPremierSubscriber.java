package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.VaultPurchaseWarning;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
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
 * Tests purchasing a multiple edition premier vault entitlement with a premier
 * origin access subscription redirects the user to the 'Game library' and opens
 * the OGD of the game
 *
 * @author cdeaconu
 */
public class OAPurchaseMultipleEditionPremierVaultPremierSubscriber extends EAXVxTestTemplate {
    
    @TestRail(caseId = 3567981)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseMultipleEditionPremierVaultPremierSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String offerId = entitlement.getOfferId();
        final String ocdPath = entitlement.getOcdPath();
        
        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Purchase 'Origin Premier Access'."); // 2
        logFlowPoint("Navigate to GDP of a multiple edition premier vault entitlement."); // 3
        logFlowPoint("Verify a 'Buy' option is showing in the primary CTA drop-down menu."); // 4
        logFlowPoint("Click on the drop-down 'Buy' option and verify the page redirects to the 'Offer Selection Page'."); // 5
        logFlowPoint("Click on 'Buy <price text>' CTA for an edition on the comparison table and verify the checkout flow starts."); // 6
        logFlowPoint("Complete the checkout flow and close the 'Thank you' modal."); // 7
        logFlowPoint("Verify 'View in Library' primary CTA is showing."); // 8
        logFlowPoint("Click on the 'View in Library' primary button and verify the page is navigated to 'Game Library' and opens the OGD of the game."); // 9
        logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 10
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOAPremierSubscription(driver, userAccount.getEmail()), true);
        }else {
            logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);
        }

        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyDropdownBuyNowVisible(), true);
        
        // 5
        gdpActionCTA.selectDropdownBuy();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.waitForPageToLoad();
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);
        
        // 6
        offerSelectionPage.clickPrimaryButton(ocdPath);
        VaultPurchaseWarning vaultPurchaseWarning = new VaultPurchaseWarning(driver);
        vaultPurchaseWarning.waitIsVisible();
        vaultPurchaseWarning.clickPurchaseAnywaysCTA();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);
        
        // 7
        if (OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            paymentInformationPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
            MacroPurchase.completePurchaseByPaypalOffCode(driver, entitlement.getName(), userAccount.getEmail());
            MacroPurchase.handleThankYouPage(driver);
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