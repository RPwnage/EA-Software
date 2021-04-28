package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.ChangeOriginAccessMembershipDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.ReviewChangesMembershipDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.UpdatedMembershipConfirmationDialog;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Origin Access' membership plan change
 *
 * @author cbalea
 */
public class OAMembershipPlanChangeBilling extends EAXVxTestTemplate {

    @TestRail(caseId = 4383269)
    @Test(groups = {"originaccess", "full_regression", "release_smoke"})
    public void testMembershipPlanChangeBilling(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Launch and log into Origin with a user with monthly subscription"); // 1
        logFlowPoint("Hover over 'Origin Access' tab and click on 'Manage my membership' and verify the 'Change Membership' modal is displayed"); // 2
        logFlowPoint("Click on 'Change Billing' CTA and verify the 'Review Changes' modal is displayed"); // 3
        logFlowPoint("Verify the modal displays the correct current membership plan"); // 4
        logFlowPoint("Verify the modal displays the price change from monthly to yearly"); // 5
        logFlowPoint("Click the 'Confirm Change' CTA and verify the confirmation modal is displayed"); // 6
        logFlowPoint("Click the 'Close' CTA and verify the modal is dismissed"); // 7
        logFlowPoint("Open 'Manage my Membership' modal and verify the user is notified of the upcoming changes to their subscription plan"); // 8
        logFlowPoint("Open the 'Review Changes' modal and verify the text changes from monthly to yearly"); // 9

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        MacroOriginAccess.purchasePremierMonthlySubscription(driver);

        // 2
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.clickManageMyMembershipLink();
        ChangeOriginAccessMembershipDialog changeOriginAccessMembershipDialog = new ChangeOriginAccessMembershipDialog(driver);
        logPassFail(changeOriginAccessMembershipDialog.waitIsVisible(), true);

        // 3
        changeOriginAccessMembershipDialog.clickChangeBillingCTA();
        ReviewChangesMembershipDialog reviewChangesMembershipDialog = new ReviewChangesMembershipDialog(driver);
        logPassFail(reviewChangesMembershipDialog.waitIsVisible(), true);

        // 4
        boolean billingPlanMonthly = reviewChangesMembershipDialog.verifyCurrentMembershipBillingPlanMonthlyText();
        logPassFail(reviewChangesMembershipDialog.verifyCurrentMonthlyMembershipPlan(), true);

        // 5
        String currentPlanPrice = reviewChangesMembershipDialog.getCurrentMembershipPlanPrice();
        String newPlanPrice = reviewChangesMembershipDialog.getNewMembershipPlanPrice();
        logPassFail(!currentPlanPrice.equals(newPlanPrice), true);

        // 6
        reviewChangesMembershipDialog.clickConfirmChangeCTA();
        UpdatedMembershipConfirmationDialog updatedMembershipConfirmationDialog = new UpdatedMembershipConfirmationDialog(driver);
        logPassFail(updatedMembershipConfirmationDialog.waitIsVisible(), true);

        // 7
        updatedMembershipConfirmationDialog.clickCloseCTA();
        logPassFail(updatedMembershipConfirmationDialog.waitIsClose(), true);

        // 8
        navigationSidebar.clickManageMyMembershipLink();
        changeOriginAccessMembershipDialog.waitForVisible();
        logPassFail(changeOriginAccessMembershipDialog.verifyYearlyMembershipPlanBillingInfo(), false);

        // 9
        changeOriginAccessMembershipDialog.clickChangeBillingCTA();
        reviewChangesMembershipDialog.waitForVisible();
        boolean billingPlanYearly = reviewChangesMembershipDialog.verifyCurrentMembershipBillingPlanYearlyText();
        logPassFail(billingPlanYearly && billingPlanMonthly, true);

        softAssertAll();
    }
}
