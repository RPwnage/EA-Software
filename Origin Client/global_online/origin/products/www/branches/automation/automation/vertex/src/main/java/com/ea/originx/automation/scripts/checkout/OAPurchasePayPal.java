package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.PayPalDialog;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the ability to purchase a game through paypal
 *
 * NEEDS UPDATE TO GDP
 *
 * @author JMittertreiner
 */
public class OAPurchasePayPal extends EAXVxTestTemplate {

    @TestRail(caseId = 11911)
    @Test(groups = {"checkout", "full_regression", "int_only", "allfeaturesmoke"})
    public void testPurchasePayPal(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        boolean isClient = ContextHelper.isOriginClientTesing(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo game = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        logFlowPoint("Launch Origin and login a new user " + userAccount.getUsername()); // 1
        logFlowPoint("Navigate to " + game.getName() + "'s PDP and start the checkout flow"); // 2
        logFlowPoint("Select PayPal as the payment method and Verify that the Paypal window opens in a new window/tab"); // 3
        logFlowPoint("Verify the main window returns to the store page"); // 4
        logFlowPoint("Complete the PayPal purchase and verify that you are brought to the Review Order Page"); // 5
        logFlowPoint("Confirm the review page and verify you are brought back to the PDP"); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user account: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user account: " + userAccount.getUsername());
        }

        // 2
      //  MacroPDP.loadPdpPage(driver, game);
        //    new PDPHeroActionCTA(driver).clickBuyButton();
        PaymentInformationPage paymentPage = new PaymentInformationPage(driver);
        paymentPage.waitForPaymentInfoPageToLoad();
        if (paymentPage.verifyPaymentInformationReached()) {
            logPass("Successfully navigated to " + game.getName() + "'s PDP and started the checkout flow");
        } else {
            logFailExit("Failed to navigate to " + game.getName() + "'s PDP or start the checkout flow");
        }

        // 3
        paymentPage.selectPayPal();
        if (paymentPage.verifyPayPalSelected()) {
            logPass("Successfully selected PayPal");
        } else {
            logFailExit("Failed to select PayPal");
        }

        // 4
        paymentPage.clickContinueToPayPal();
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        Waits.pollingWait (() -> reviewOrderPage.verifyReviewOrderPageReached());
        reviewOrderPage.clickPayNowAndContinueToThirdPartyVendor();
        driver.switchTo().defaultContent();
//        if (new PDPHeroActionCTA(driver).verifyPDPHeroReached()) {
//            logPass("The main page was brought back to the store");
//        } else {
//            logFail("The main page was not brought back to the store");
//        }

        // 5
        PayPalDialog dialog = new PayPalDialog(driver);
        dialog.waitForVisible();
        dialog.loginAndCompletePurchase();
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.waitForThankYouPageToLoad();
        if (thankYouPage.verifyThankYouPageReached()) {
            logPass("Was brought to the thank you page");
        } else {
            logFail("Was not brought to the thank you page");
        }

        // 6
        thankYouPage.clickCloseButton();
//        if (new PDPHeroActionCTA(driver).verifyPDPHeroReached()) {
//            logPass("Successfully was brought back to the PDP");
//        } else {
//            logFail("Was not brought back to the PDP");
//        }

        client.stop();
        softAssertAll();
    }
}
