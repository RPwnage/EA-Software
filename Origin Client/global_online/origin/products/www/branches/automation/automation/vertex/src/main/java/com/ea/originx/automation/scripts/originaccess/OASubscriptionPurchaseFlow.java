package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
 * Tests the different aspects of the Origin Access sign-up flow on PC
 *
 * @author alcervantes
 */
public class OASubscriptionPurchaseFlow extends EAXVxTestTemplate {

    @TestRail(caseId = 11061)
    @Test(groups = {"originaccess", "full_regression"})
    public void testSubscriptionPurchaseFlow(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount =  AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Login to a non subscriber account, navigate to the Origin Access landing page by clicking on 'Origin Access' in the left nav");//1
        logFlowPoint("Scroll to the bottom, verify Origin Access Terms and Conditions are present on the 'Origin Access' landing page.");//2
        logFlowPoint("Select on 'Become A Premier Member' CTA, verify the 'Plan Selection' modal is shown to the player");//3
        logFlowPoint("Select a subscription plan (only if more than one is merchandised), verify there is 1 'Best Value' subscription plan");//4
        logFlowPoint("Click on yearly plan type, verify the selected plan is indicated by a green border and check mark");//5
        logFlowPoint("Click on monthly plan type, verify the selected plan is indicated by a green border and check mark");//6
        logFlowPoint("Click the X button on the modal, verify user can exit plan selection by selecting the X button on the modal");//7
        logFlowPoint("Click the 'Join Now' CTA again");//8
        logFlowPoint("Verify user can exit plan selection by selecting outside the modal");//9
        logFlowPoint("Click the 'Join Now' CTA again");//10
        logFlowPoint("Select 'Next', verify 'Payment Info' page is reached");//11
        logFlowPoint("Verify user is brought to checkout flow");//12
        logFlowPoint("Complete the check out flow");//13
        logFlowPoint("View the Mini profile in the left nav, verify completing checkout subscribes user to 'Origin Access' by applying a badge to their mini profile");//14

        //1
        WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        new NavigationSidebar(driver).clickOriginAccessLink();
        OriginAccessPage originAccessPage = new OriginAccessPage(driver);
        originAccessPage.waitForPageToLoad();
        logPassFail(originAccessPage.verifyPageReached(), true);

        //2
        originAccessPage.scrollToBottom();
        Waits.sleep(1000);
        logPassFail(originAccessPage.verifyLegalSectionVisible(), true);

        //3
        originAccessPage.scrollToTop();
        Waits.sleep(1000);
        originAccessPage.clickJoinPremierSubscriberCta();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        selectOriginAccessPlanDialog.waitForYearlyPlanAndStartTrialButtonToLoad();
        logPassFail(selectOriginAccessPlanDialog.isDialogVisible(), true);

        //4
        selectOriginAccessPlanDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN);
        Waits.sleep(1000);
        logPassFail(selectOriginAccessPlanDialog.isBestValueTextVisible(), true);

        //5
        logPassFail(selectOriginAccessPlanDialog.isYearlyPlanSelected(), true);

        //6
        selectOriginAccessPlanDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN);
        Waits.sleep(1000);
        logPassFail(selectOriginAccessPlanDialog.isMonthlyPlanSelected(), true);

        //7
        selectOriginAccessPlanDialog.clickCloseCircle();
        Waits.sleep(1000);
        logPassFail(selectOriginAccessPlanDialog.waitIsClose(), true);

        //8
        originAccessPage.clickJoinPremierSubscriberCta();
        Waits.sleep(1000);
        logPassFail(selectOriginAccessPlanDialog.isDialogVisible(), true);

        //9
        selectOriginAccessPlanDialog.clickOutsideDialogWindow();
        logPassFail(selectOriginAccessPlanDialog.waitIsClose(), true);

        //10
        originAccessPage.clickJoinPremierSubscriberCta();
        Waits.sleep(1000);
        logPassFail(selectOriginAccessPlanDialog.isDialogVisible(), true);

        //11
        selectOriginAccessPlanDialog.clickNext();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPageToLoad();
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);

        //12
        MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        reviewOrderPage.waitForPageToLoad();
        logPassFail(reviewOrderPage.verifyReviewOrderPageReached(), true);

        //13
        reviewOrderPage.clickStartMembershipButton();
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        originAccessNUXFullPage.waitForPageToLoad();
        logPassFail(originAccessNUXFullPage.verifyPageReached(), true);
        Waits.sleep(5000);
        originAccessNUXFullPage.clickSkipToOriginAccessCollection();

        //14
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        logPassFail(miniProfile.verifyOriginAccessBadgeVisible(), true);

        softAssertAll();
    }
}
