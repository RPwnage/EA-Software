package com.ea.originx.automation.scripts.originaccess;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Review Order when purchasing Origin Access
 *
 * @author cvanichsarn
 */
public class OAOriginAccessReviewOrder extends EAXVxTestTemplate {

    @TestRail(caseId = 11011)
    @Test(groups = {"originaccess", "client_only", "full_regression"})
    public void testOriginAccessReviewOrder(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String username = userAccount.getUsername();

        //
        logFlowPoint("Login as: " + username); //1
        logFlowPoint("Navigate to the 'Origin Access' page"); //2
        logFlowPoint("Click 'Join' on the Origin Access Page"); //3
        logFlowPoint("Enter Billing Information"); //4
        logFlowPoint("Click on Review Order button"); //5
        logFlowPoint("Verify TOS text on Review Order Page"); //6
        logFlowPoint("Verify all links in the TOS area open an external browser"); //7
        logFlowPoint("Verify the correct amount of tax is applied"); //8
        logFlowPoint("Verify Edit Billing Information link opens a new modal"); //9

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        OriginAccessPage accessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        logPassFail(accessPage.verifyPageReached(), true);

        //3
        accessPage.clickJoinBasicButton();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        if (selectOriginAccessPlanDialog.waitIsVisible()) {
            selectOriginAccessPlanDialog.clickNext();
        }
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInfoPage.verifyPaymentInformationReached(), true);

        //4
        paymentInfoPage.enterCreditCardDetails();
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.isProceedToReviewOrderButtonEnabled()), true);

        //5
        paymentInfoPage.clickOnProceedToReviewOrderButton();
        ReviewOrderPage reviewOrder = new ReviewOrderPage(driver);
        reviewOrder.waitForPageToLoad();
        logPassFail(Waits.pollingWait(() -> reviewOrder.verifyReviewOrderPageReached()), true);

        //6
        logPassFail(Waits.pollingWait(() -> reviewOrder.verifyOriginAccessTermsOfServiceText()), false);

        //7
        logPassFail(Waits.pollingWait(() -> reviewOrder.verifyAllTermsOfServiceLinks()), false);

        //8
        logPassFail(Waits.pollingWait(() -> reviewOrder.verifyTaxIsDisplayed()), false);

        //9
        reviewOrder.clickEditPaymentOptionsButton();
        logPassFail(paymentInfoPage.verifyPaymentInformationReached(), true);

        softAssertAll();
    }
}
