package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftMessageDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.utils.AccountService;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test sending and receiving a gift
 * <p>
 * "Memories are forever." -- Lois Lowry, The Giver
 *
 * @author jmittertreiner
 * @author cbalea
 * @author sbentley
 */
public class OARedeemGift extends EAXVxTestTemplate {
    
    @TestRail(caseId = 11768)
    @Test(groups = {"gifting", "full_regression", "gifting_smoke", "int_only", "allfeaturesmoke", "client_only"})
    public void testRedeemGift(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount gifter = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_USA);
        AccountService.registerNewUserThroughREST(gifter, null);
        final UserAccount giftee = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_USA);
        AccountService.registerNewUserThroughREST(giftee, null);
        EACoreHelper.overrideCountryTo(CountryInfo.CountryEnum.USA, client.getEACore());
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String gifteeName = giftee.getUsername();
        gifter.addFriend(giftee);
        
        logFlowPoint("Log into Origin");    // 1
        logFlowPoint("Navigate to a PDP");  // 2
        logFlowPoint("Click on the drop down next to Purchase CTA and select 'Gift this Game'");    // 3
        logFlowPoint("Enter the name of the person you want to gift to, a message, then click submit and verify user is brought to checkout flow"); // 4
        logFlowPoint("Verify Credit Card are an available payment method"); // 5
        logFlowPoint("Verify PayPal is an available payment method");   // 6
        logFlowPoint("Verify EA Wallet is an available payment method, if valid for the locale");   // 7
        logFlowPoint("Enter billing information and verify information is valid");  // 8
        logFlowPoint("Advance through the checkout flow and verify line stating purchse is a gift");    // 9
        logFlowPoint("Click submit your order and verify order number appears on Thank You page");  // 10
        logFlowPoint("Verify text appears confirming purchase as a gift");  // 11
        
        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/usa/en-us");

        logPassFail(MacroLogin.startLogin(driver, gifter), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        // 3
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        new OfferSelectionPage(driver).clickPrimaryButton(entitlement.getOcdPath());
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);
        
        // 4
        sleep(2000);
        friendsSelectionDialog.selectRecipient(giftee);
        friendsSelectionDialog.clickNext();
        GiftMessageDialog giftMessageDialog = new GiftMessageDialog(driver);
        final String message = "This is a message";
        giftMessageDialog.enterMessage(message);
        giftMessageDialog.clickNext();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);
        
        // 5
        logPassFail(paymentInformationPage.verifyPaymentOptionCreditCardVisible(), false);
        
        // 6
        logPassFail(paymentInformationPage.verifyPaymentOptionPayPalVisible(), false);
        
        // 7
        logPassFail(paymentInformationPage.verifyPaymentOptionWalletVisible(), false);

        // 8
        paymentInformationPage.enterCreditCardDetails();
        Waits.sleep(2000); // Wait for credit card details to be entered and for fields to be verified
        logPassFail(paymentInformationPage.isProceedToReviewOrderButtonEnabled(), true);

        // 9
        paymentInformationPage.clickOnProceedToReviewOrderButton();
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        reviewOrderPage.waitForReviewOrderPageToLoad();
        logPassFail(reviewOrderPage.verifyGiftForUser(gifteeName), false);

        // 10
        MacroPurchase.handleReviewOrderPage(driver);
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        thankYouPage.waitForThankYouPageToLoad();
        logPassFail(thankYouPage.getOrderNumber() != null, false);

        // 11
        logPassFail(thankYouPage.verifyGiftForUser(gifteeName), false);

        softAssertAll();
    }
}