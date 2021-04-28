package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
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
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test upsell and upgrade from review order page
 *
 * @author rchoi
 */
public class OAUpsellUpgradePurchase extends EAXVxTestTemplate {

    @TestRail(caseId = 3068476)
    @Test(groups = {"checkout_smoke", "checkout", "allfeaturesmoke"})
    public void testUpsellUpgradePurchase(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo standardEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String standardEntitlementName = standardEntitlement.getName();
        
        logFlowPoint("Login to Origin with new user account"); // 1
        logFlowPoint("Navigate to GDP for a base game with an upgradable offer"); //2
        logFlowPoint("Get to payment information page by clicking 'Get the Game' button for the base game"); //3
        logFlowPoint("Enter valid credit card information"); //4
        logFlowPoint("Click order button and wait for review order page "); //5
        logFlowPoint("Verify upgrade entitlement and upsell entitlement exist as a cart item for upgrading and upselling"); //6
        logFlowPoint("Add extra content as upsell entitlement and verify the extra content is in the cart"); //7
        logFlowPoint("Upgrade the base game by and verify upgraded entitlement is on the review order page instead of the base game"); //8
        logFlowPoint("Verify total price is same as tax + price for all entitlements"); //9
        logFlowPoint("Clicked on 'Purchase' button and navigate to 'Thank You' page successfully"); //10
        logFlowPoint("Verify upgraded entitlement and extra content still exist on 'Thank you' page"); //11
        logFlowPoint("Navigate to the 'Game Library' page"); //12
        logFlowPoint("Verify upgraded entitlement is in 'Game Library'"); //13
        logFlowPoint("Verify upsell entitlement is in the 'Game Library'"); //14

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, standardEntitlement), true);

        //3
        logPassFail(MacroPurchase.clickPurchaseEntitlementAndWaitForPurchaseDialog(driver, standardEntitlement), true);

        //4
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.enterCreditCardDetails();
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.isProceedToReviewOrderButtonEnabled()), true);

        //5
        paymentInfoPage.clickOnProceedToReviewOrderButton();
        ReviewOrderPage reviewOrder = new ReviewOrderPage(driver);
        reviewOrder.waitForPageToLoad();
        logPassFail(Waits.pollingWait(() -> reviewOrder.verifyReviewOrderPageReached()), true);

        //6
        // checking if list of optional purchase for the base game entitlement exists
        logPassFail(reviewOrder.verifyUpsellUpgradeCartItemExists(), true);

        //7
        // getting name and offerID from review order page
        final String upgradeEntitlementName = reviewOrder.getUpgradeEntitlementName();
        final String upsellEntitlementName = reviewOrder.getUpsellEntitlementName();
        reviewOrder.clickUpsellEntitlement();
        Waits.sleep(3000); // wait for animation to be finished. getting error for 1000 and 2000
        logPassFail(reviewOrder.verifyEntitlementExistsForReviewingOrder(upsellEntitlementName), true);

        //8 upgrading base game entitlement (upgrade)
        reviewOrder.clickUpgradeEntitlement();
        Waits.sleep(3000); // wait for animation to be finished. getting error for 1000 and 2000
        logPassFail(reviewOrder.verifyEntitlementExistsForReviewingOrder(upgradeEntitlementName) && !reviewOrder.verifyEntitlementExistsForReviewingOrder(standardEntitlementName), true);

        //9
        logPassFail(reviewOrder.verifyTotalCost(), true);

        //10
        reviewOrder.clickCompleteOrderButton();
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.waitForThankYouPageToLoad();
        logPassFail(Waits.pollingWait(() -> thankYouPage.verifyThankYouPageReached()), true);

        //11
        logPassFail(thankYouPage.verifyPurchasedEntitlementExists(upgradeEntitlementName) && thankYouPage.verifyPurchasedEntitlementExists(upsellEntitlementName), true);

        //12
        thankYouPage.clickCloseButton();
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //13
        logPassFail(gameLibrary.isGameTileVisibleByName(upgradeEntitlementName), true);

        //14
        logPassFail(gameLibrary.isGameTileVisibleByName(upsellEntitlementName), true);

        softAssertAll();
    }
}