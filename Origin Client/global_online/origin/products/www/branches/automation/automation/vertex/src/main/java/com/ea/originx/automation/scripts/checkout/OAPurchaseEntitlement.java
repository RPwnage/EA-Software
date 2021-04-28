package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test the checkout flow of entitlement in the store
 *
 * @author svaghayenegar
 */
public class OAPurchaseEntitlement extends EAXVxTestTemplate {

    public void testPurchaseEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Launch Origin and register as a new user" + userAccount.getUsername());
        logFlowPoint("Navigate to GDP Page");//2
        logFlowPoint("Click on Buy Button");//3
        logFlowPoint("Enter Credit Card Information"); //4
        logFlowPoint("Click on Proceed to Review Order button"); //5
        logFlowPoint("Click on Purchase"); //6
        logFlowPoint("Navigate to game library"); //7
        logFlowPoint("Verify Game appears in the game library after purchase"); //8

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully created user and logged in as: " + userAccount.getEmail());
        } else {
            logFailExit("Could not create user");
        }

        //2
        if (MacroGDP.loadGdpPage(driver, entitlement)) {
            logPass("Successfully navigated to GDP page");
        } else {
            logFailExit("Could not navigate to the Game Description Page");
        }

        //3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickBuyCTA();
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        if (Waits.pollingWait(() -> paymentInfoPage.verifyPaymentInformationReached())) {
            logPass("Clicked on 'Buy' button and Navigated to payment information page");
        } else {
            logFailExit("Payment Information Page failed to open");
        }

        //4
        paymentInfoPage.enterCreditCardDetails();
        if (Waits.pollingWait(() -> paymentInfoPage.isProceedToReviewOrderButtonEnabled())) {
            logPass("Credit Card information entered successfully");
        } else {
            logFailExit("Credit Card information not entered properly");
        }

        //5
        paymentInfoPage.clickOnProceedToReviewOrderButton();
        ReviewOrderPage reviewOrder = new ReviewOrderPage(driver);
        reviewOrder.waitForPageToLoad();
        if (Waits.pollingWait(() -> reviewOrder.verifyReviewOrderPageReached())) {
            logPass("Clicked on Proceed to Review Order and successfully navigated to Review Order Page");

        } else {
            logFailExit("Could not navigate to Review Order page");
        }

        //6
        reviewOrder.clickCompleteOrderButton();
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        if (Waits.pollingWait(() -> thankYouPage.verifyThankYouPageReached())) {
            logPass("Clicked on 'Purchase' button and navigated to Thank You Page successfully");
        } else {
            logFailExit("Could not purchase the product");
        }

        //7
        thankYouPage.clickCloseButton();
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Successfully navigated to the 'Game Library' page");
        } else {
            logFailExit("Failed to navigate to the 'Game Library' page");
        }

        //8
        if (gameLibrary.isGameTileVisibleByName(entitlementName)) {
            logPass(entitlementName + " appears in game Library after a successful purchase");
        } else {
            logFailExit(entitlementName + " did not appear in game library after successfull purchase");
        }
        softAssertAll();
    }

}
