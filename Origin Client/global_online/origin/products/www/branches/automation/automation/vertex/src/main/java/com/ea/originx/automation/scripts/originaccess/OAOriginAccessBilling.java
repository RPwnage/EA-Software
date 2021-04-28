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
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the billing options available when purchasing Origin Access.
 *
 * @author glivingstone
 */
public class OAOriginAccessBilling extends EAXVxTestTemplate {

    @TestRail(caseId = 11010)
    @Test(groups = {"originaccess", "full_regression"})
    public void testOriginAccessBilling(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String username = userAccount.getUsername();

        logFlowPoint("Login as: " + username); //1
        logFlowPoint("Navigate to the 'Origin Access' page"); //2
        logFlowPoint("Click 'Join' on the Origin Access Page"); //3
        logFlowPoint("Verify There are Only PayPal and Credit Card options to purchase Origin Access"); //4
        logFlowPoint("Verify the Billing Information Section is the Same as the Store Checkout"); //5

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
        logPassFail(paymentInfoPage.verifyPaymentMethods(), true);

        //5
        paymentInfoPage.enterCreditCardDetails();
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.isProceedToReviewOrderButtonEnabled()), true);

        softAssertAll();
    }

}
