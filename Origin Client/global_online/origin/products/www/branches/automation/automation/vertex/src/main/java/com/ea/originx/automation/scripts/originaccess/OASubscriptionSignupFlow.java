package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Premier subscription purchase flow on PC for a non subscriber
 *
 * @author alcervantes
 */
public class OASubscriptionSignupFlow extends EAXVxTestTemplate {

    @TestRail(caseId = 2999019)
    @Test(groups = {"originaccess", "full_regression"})
    public void testSubscriptionSignupFlow(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount =  AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Navigate to or launch Origin and log in");//1
        logFlowPoint("Click on the Origin Access tab on the left nav, verify the user is brought to the Origin Access Landing Page");//2
        logFlowPoint("Click on 'Become A Premier Memeber' CTA, verify a Premier subscription selection modal shows up");//3
        logFlowPoint("Click on monthly SKU and verify that it's highlighted");//4
        logFlowPoint("Click on yearly SKU and verify that it's highlighted");//5
        logFlowPoint("Click on 'Next CTA', verify that it takes the user to 'Review Order' page");//6
        logFlowPoint("Click on the 'Start Membership' CTA, verify that the Nux Flow for Premium Subscriber is initiated");//7

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new NavigationSidebar(driver).clickOriginAccessLink();
        OriginAccessPage originAccessPage = new OriginAccessPage(driver);
        originAccessPage.waitForPageToLoad();
        logPassFail(originAccessPage.verifyPageReached(), true);

        //3
        originAccessPage.clickJoinPremierSubscriberCta();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        selectOriginAccessPlanDialog.waitForYearlyPlanAndStartTrialButtonToLoad();
        logPassFail(selectOriginAccessPlanDialog.isDialogVisible(), true);

        //4
        selectOriginAccessPlanDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN);
        Waits.sleep(600);
        logPassFail(selectOriginAccessPlanDialog.isMonthlyPlanSelected(), true);

        //5
        selectOriginAccessPlanDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN);
        Waits.sleep(600);
        logPassFail(selectOriginAccessPlanDialog.isYearlyPlanSelected(), true);

        //6
        selectOriginAccessPlanDialog.clickNext();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPageToLoad();
        paymentInformationPage.waitForPaymentInfoPageToLoad();

        MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        reviewOrderPage.waitForPageToLoad();
        logPassFail(reviewOrderPage.verifyReviewOrderPageReached(), true);

        //7
        reviewOrderPage.clickStartMembershipButton();
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        originAccessNUXFullPage.waitForPageToLoad();
        logPassFail(originAccessNUXFullPage.verifyPageReached(), true);

        softAssertAll();
    }
}
