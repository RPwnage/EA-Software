package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.store.*;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
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
 * Test the core purchase flow on mobile: Store Home -> Search -> PDP -> Cart ->
 * Checkout
 *
 * NEEDS UPDATE TO GDP
 *
 * @author jmittertreiner
 */
public class OAPurchaseFlow extends EAXVxTestTemplate {

    @Test(groups = {"checkout", "mobile"})
    public void testPurchaseFlow(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Launch Origin and register as a new user: " + userAccount.getUsername()); // 1
        logFlowPoint("Search for " + entitlementName + " and click on the result to go to the entitlement's PDP"); // 2
        logFlowPoint("Click on the buy button"); // 3
        logFlowPoint("Enter credit card information"); // 4
        logFlowPoint("Click on 'Proceed to Review Order' button"); // 5
        logFlowPoint("Click on Purchase"); // 6
        logFlowPoint("Navigate to game library"); // 7
        logFlowPoint("Verify game appears in the game library after purchase"); // 8

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully created user and logged in as: " + userAccount.getEmail());
        } else {
            logFailExit("Could not create user");
        }

        // 2
        //MacroPDP.loadPdpPageBySearch(driver, entitlement);
//        PDPPage pdp = new PDPPage(driver);
//        if (pdp.verifyPDPPageReached()) {
//            logPass("Successfully navigated to " + entitlementName + "'s PDP");
//        } else {
//            logFailExit("Could not navigate to " + entitlementName + "'s PDP");
//        }

        // 3
       // new PDPHeroActionCTA(driver).clickBuyButton();
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        if (Waits.pollingWait(paymentInfoPage::verifyPaymentInformationReached)) {
            logPass("Clicked on 'Buy' button and Navigated to payment information page");
        } else {
            logFailExit("Payment Information Page failed to open");
        }

        // 4
        paymentInfoPage.enterCreditCardDetails();
        if (Waits.pollingWait(paymentInfoPage::isProceedToReviewOrderButtonEnabled)) {
            logPass("Credit Card information entered successfully");
        } else {
            logFailExit("Credit Card information not entered properly");
        }

        // 5
        paymentInfoPage.clickOnProceedToReviewOrderButton();
        ReviewOrderPage reviewOrder = new ReviewOrderPage(driver);
        reviewOrder.waitForPageToLoad();
        if (Waits.pollingWait(reviewOrder::verifyReviewOrderPageReached)) {
            logPass("Clicked on Proceed to Review Order and successfully navigated to Review Order Page");

        } else {
            logFailExit("Could not navigate to Review Order page");
        }

        // 6
        reviewOrder.clickCompleteOrderButton();
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        if (Waits.pollingWait(thankYouPage::verifyThankYouPageReached)) {
            logPass("Clicked on 'Purchase' button and navigated to Thank You Page successfully");
        } else {
            logFailExit("Could not purchase the product");
        }

        // 7
        thankYouPage.clickCloseButton();
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        // 8
        if (gameLibrary.isGameTileVisibleByName(entitlementName)) {
            logPass(entitlementName + " appears in game Library after a successful purchase");
        } else {
            logFailExit(entitlementName + " did not appear in game library after successfull purchase");
        }
        softAssertAll();
    }

}
