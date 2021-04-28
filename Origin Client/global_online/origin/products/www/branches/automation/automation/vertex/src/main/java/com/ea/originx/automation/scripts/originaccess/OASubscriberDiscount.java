package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.SoftDependencySuggestionDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests OA Subscriber discount through the purchase flow
 *
 * @author caleung
 */
public class OASubscriberDiscount extends EAXVxTestTemplate {

    @TestRail(caseId = 1016692)
    @Test(groups = {"originaccess", "release_smoke", "int_only"})
    public void testSubscriberDiscount(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_MY_FIRST_PET);

        logFlowPoint("Create an account and log into Origin with a newly registered account."); // 1
        logFlowPoint("Purchase an 'Origin Access' subscription."); // 2
        logFlowPoint("Navigate to a entitlement that has a 10% discount, verify discount shows, green discount text shows, and"
                + " the original price is striked through, and then click 'Buy'."); // 3
        logFlowPoint("Enter payment information, verify that the price is correct, and verify the strike through original price is displayed."); // 4
        logFlowPoint("Verify the total price displayed is after the discount is applied."); // 5
        logFlowPoint("Click 'Pay Now' and verify the total price displayed is the discounted price."); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        if(OSInfo.getEnvironment().equals("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            logPassFail(MacroRedeem.redeemOABasicSubscription(driver, userAccount.getEmail()), true);
        } else {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }
        // 3
        MacroGDP.loadGdpPage(driver, entitlementInfo);
        logPassFail(new GDPHeroActionDescriptors(driver).verifyOriginAccessDiscountIsVisible(), false);

        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        String expectedPrice = gdpActionCTA.getPriceFromBuyButton();
        gdpActionCTA.clickBuyCTA();
        SoftDependencySuggestionDialog softDependencySuggestionDialog = new SoftDependencySuggestionDialog(driver);
        if (softDependencySuggestionDialog.waitIsVisible()) {
            softDependencySuggestionDialog.clickContinueWithoutAddingDlcButton();
        }

        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        if(OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            paymentInformationPage.switchToPaymentByPaypalAndSkipToReviewOrderPage();
        } else {
            MacroPurchase.handlePaymentInfoPage(driver);
        }
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        reviewOrderPage.waitForReviewOrderPageToLoad();
        // verify price after discount is correct, green "Origin Access Discount" text is displayed, and original price is striked through
        boolean isPriceAfterDiscountCorrect = reviewOrderPage.verifyPriceAfterDiscountIsCorrect(expectedPrice);
        boolean isOriginAccessDiscountVisible = reviewOrderPage.verifyOriginAccessDiscountIsVisible();
        boolean isOriginalPriceIsStrikedThrough = reviewOrderPage.verifyOriginalPriceIsStrikedThrough();
        logPassFail(isPriceAfterDiscountCorrect && isOriginAccessDiscountVisible && isOriginalPriceIsStrikedThrough, false);

        // 5
        double totalPriceOnReviewPage = reviewOrderPage.getTotalCost();
        if (reviewOrderPage.verifyTotalCost()) {
            logPass("Verified total price after the discount on the review order page is correct.");
        } else {
            logFailExit("Total price after the discount on the review order page is not correct.");
        }

        // 6
        if(OSInfo.getEnvironment().equalsIgnoreCase("production")) {
            MacroPurchase.completePurchaseByPaypalOffCode(driver, entitlementInfo.getName(), userAccount.getEmail());
        } else {
            reviewOrderPage.clickPayNow();
            ThankYouPage thankYouPage = new ThankYouPage(driver);
            thankYouPage.waitForThankYouPageToLoad();
            double totalPriceOnThankYouPage = thankYouPage.getTotalAmount();
            if (Double.compare(totalPriceOnReviewPage, totalPriceOnThankYouPage) == 0) {
                logPass("Verified total price on thank you page is the discounted price.");
            } else {
                logFailExit("Total price on thank you page is not the discounted price.");
            }
        }

        softAssertAll();
    }
}
