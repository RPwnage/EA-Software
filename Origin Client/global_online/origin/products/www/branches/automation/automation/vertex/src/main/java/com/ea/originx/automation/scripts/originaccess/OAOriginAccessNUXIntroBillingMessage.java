package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
 * Tests the 'Billing' section in the 'Origin Access' NUX intro
 *
 * @author cbalea
 */
public class OAOriginAccessNUXIntroBillingMessage extends EAXVxTestTemplate {

    @TestRail(caseId = 40236)
    @Test(groups = {"originacess", "full_regression"})
    public void testOriginAccessNUXIntroBillingMessage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccountA = AccountManager.getInstance().createNewThrowAwayAccount();
        UserAccount userAccountB = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Log into Origin with a non-subscriber"); // 1
        logFlowPoint("Purchase 'Origin Access Premier' with monthly plan"); // 2
        logFlowPoint("Verify NUX intro message informs player they are billed monthly"); // 3
        logFlowPoint("Verify next billing date is correct"); // 4
        logFlowPoint("Log out and log in with a different non-subscriber"); // 5
        logFlowPoint("Purchase 'Origin Access Premier' with yearly plan"); // 6
        logFlowPoint("Verify NUX intro message informs player they are billed yearly"); // 7
        logFlowPoint("Verify next billing date is correct"); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccountA), true);

        // 2
        logPassFail(MacroOriginAccess.purchasePremierMonthlySubscriptionAndShowNux(driver), true);

        // 3
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        logPassFail(originAccessNUXFullPage.verifyBillingMonthly(), false);

        // 4
        logPassFail(originAccessNUXFullPage.verifyNextBillingMonthlyDate(), true);

        // 5
        originAccessNUXFullPage.clickSkipToOriginAccessCollection();
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, userAccountB), true);

        // 6
        logPassFail(MacroOriginAccess.purchaseOriginAccessWithPlan(driver, SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN, false, true, false), true);

        // 7
        logPassFail(Waits.pollingWait(() -> originAccessNUXFullPage.verifyBillingYearly()), true);

        // 8
        logPassFail(Waits.pollingWait(() -> originAccessNUXFullPage.verifyNextBillingYearlyDate()), true);

        softAssertAll();
    }
}
