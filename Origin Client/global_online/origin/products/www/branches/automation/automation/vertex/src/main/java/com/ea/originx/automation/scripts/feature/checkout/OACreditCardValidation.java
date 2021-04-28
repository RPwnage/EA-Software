package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
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
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

/**
 * Tests if input error is shown and order button is disabled when entering
 * invalid input or leaving empty for field on payment information page.
 *
 * @author rchoi
 */
public class OACreditCardValidation extends EAXVxTestTemplate {

    @TestRail(caseId = 3068174)
    @Test(groups = {"checkout_smoke", "checkout", "allfeaturesmoke"})
    public void testCreditCardValidation(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        String entitlementName = entitlement.getName();
        int currentMonth = Calendar.getInstance().get(Calendar.MONTH) + 1; //we add one, as it is zero-indexed

        logFlowPoint("Launch Origin and register as a new user" + userAccount.getUsername());  //1
        logFlowPoint("Navigate to GDP for " + entitlementName); //2
        logFlowPoint("Get to payment information page by clicking buy button for " + entitlementName); //3
        logFlowPoint("Verify credit card number input error exists and unable to click Order button by entering invalid number"); //4
        logFlowPoint("Verify credit card number input error exists and unable to click Order button by leaving the empty field"); //5
        logFlowPoint("Verify no credit card number input error exists by entering valid credit card number"); //6
        if (currentMonth != 1) {
            logFlowPoint("Verify expiry month input error exists and unable to click Order button by entering the past month if current month is not January."); //7
        }
        logFlowPoint("Verify no expiry month input error exists by entering valid month."); //8
        logFlowPoint("Verify CSC code input error exists and unable to click Order button by leaving the empty field"); //9
        logFlowPoint("Verify no CSC code input error exists by entering valid csc code"); //10
        logFlowPoint("Verify postal/zip code input error exists and unable to click Order button by leaving the empty field"); //11
        logFlowPoint("Verify no postal/zip code input error exists by entering valid postal/zip code"); //12
        logFlowPoint("Verify first name input error exists and unable to click Order button by leaving the empty field"); //13
        logFlowPoint("Verify no first name input error exists by entering valid first name"); //14
        logFlowPoint("Verify last name code input error exists and unable to click Order button by leaving the empty field"); //15
        logFlowPoint("Verify no last name code input error exists by entering valid last name"); //16
        logFlowPoint("Verify order button is enabled once Credit Card information is entered successfully"); //17

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.verifyBuyCTAVisible();
        gdpActionCTA.clickBuyCTA();
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.verifyPaymentInformationReached()), true);

        //4
        // entering invalid credit card number
        paymentInfoPage.inputCreditCardNumber("1111111111111111");
        logPassFail(paymentInfoPage.verifyCreditCardNumberInputError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        //5
        // leaving the credit card number field blank
        logPassFail(paymentInfoPage.verifyCreditCardNumberBlankError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        //6
        // entering valid credit card number
        paymentInfoPage.inputCreditCardNumber("4024007141018613");
        logPassFail(!paymentInfoPage.verifyCreditCardNumberErrorMessage(), true);

        //7
        // we check expiry month input error by selecting 1 ( = January) if current month is not January
        paymentInfoPage.inputCreditCardExpiryMonth("01");
        if (new SimpleDateFormat("M").format(new Date()) != "1") {
            logPassFail(paymentInfoPage.verifyExpiryMonthInputError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);
        }

        //8
        // entering valid expiry month
        paymentInfoPage.inputCreditCardExpiryMonth("12");
        logPassFail(!paymentInfoPage.verifyExpiryMonthErrorMessage(), true);

        //9
        // leaving the csc code field blank
        logPassFail(paymentInfoPage.verifyCSCCodeBlankError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        //10
        // entering valid csc code
        paymentInfoPage.inputCreditCardCSC("000");
        logPassFail(!paymentInfoPage.verifyCSCCodeErrorMessage(), true);

        //11
        // leaving the postal/zip code field blank
        logPassFail(paymentInfoPage.verifyPostalZipCodeBlankError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        //12
        //  entering valid postal/zip code
        paymentInfoPage.inputZipCode( "V5G 4X1");
        logPassFail(!paymentInfoPage.verifyPostalZipCodeErrorMessage(), true);

        //13
        // leaving the first name field blank
        logPassFail(paymentInfoPage.verifyFirstNameInputBlankError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        //14
        //  entering valid first name
        paymentInfoPage.inputFirstName("AutomatedTester");
        logPassFail(!paymentInfoPage.verifyFirstNameErrorMessage(), true);

        //15
        // leaving the last name field blank
        logPassFail(paymentInfoPage.verifyLastNameInputBlankError() && !paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        //16
        //  entering valid last name
        paymentInfoPage.inputLastName("Automation");
        logPassFail(!paymentInfoPage.verifyLastNameErrorMessage(), true);

        //17
        // checking if all inputs are valid
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.isProceedToReviewOrderButtonEnabled()), true);

        softAssertAll();
    }

}
