package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests entitlement purchasing using an expired credit card
 *
 * @author rchoi
 */
public class OAExpiredCreditCard extends EAXVxTestTemplate {

    @TestRail(caseId = 3068178)
    @Test(groups = {"checkout_smoke", "checkout", "allfeaturesmoke"})
    public void testExpiredCreditCardDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        final String entitlementName = entitlement.getName();

        // account which has an expired credit card
        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_CREDIT_EXPIRED);

        logFlowPoint("Login to Origin with a user with an Origin Access subscription and an expired credit card"); // 1
        logFlowPoint("Navigate to a PDP page for an entitlement to purchase"); // 2
        logFlowPoint("Click the 'Buy' button and open the 'Payment Information' page"); // 3
        logFlowPoint("Verify an error message appears which contains the word 'expired'"); // 4
        logFlowPoint("Verify that the 'Proceed to Review Order' button is disabled"); // 5
        logFlowPoint("Select the expired credit card and verify there is still an error message which contains the word 'expired'"); // 6
        logFlowPoint("Verify that the 'Proceed to Review Order' button is still disabled"); // 7
        logFlowPoint("Close the 'Payment Information' dialog and navigate to the 'Game Library' page"); // 8
        logFlowPoint("Verify the entitlement is not in the 'Game Library'"); // 9

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        new GDPActionCTA(driver).clickBuyCTA();
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.verifyPaymentInformationReached()), true);

        // 4
        logPassFail(paymentInfoPage.verifyCreditCardExpiredText(), true);

        // 5
        logPassFail(!paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        // 6
        // manually select the existing expired credit card
        paymentInfoPage.clickSavedCreditCardRadioButton();
        logPassFail(paymentInfoPage.verifyCreditCardExpiredText(),true);

        // 7
        logPassFail(!paymentInfoPage.isProceedToReviewOrderButtonEnabled(), true);

        // 8
        paymentInfoPage.clickCloseButton();
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        // 9
        logPassFail(!gameLibrary.isGameTileVisibleByName(entitlementName), true);

        softAssertAll();
    }
}