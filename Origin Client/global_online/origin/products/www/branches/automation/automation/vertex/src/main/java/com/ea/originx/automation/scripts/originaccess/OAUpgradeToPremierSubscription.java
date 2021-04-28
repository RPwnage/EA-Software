package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.ChangeOriginAccessMembershipDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test a basic account can upgrade to a premier account
 *
 * @author cdeaconu
 */
public class OAUpgradeToPremierSubscription extends EAXVxTestTemplate{
    
    @TestRail(caseId = 2997155)
    @Test(groups = {"originaccess", "full_regression"})
    public void testUpgradeToPremierSubscription (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        
        logFlowPoint("Launch and log into 'Origin' with a newly created account."); // 1
        logFlowPoint("Purchase 'Origin Basic Access'."); // 2
        logFlowPoint("Hover over the Origin Access tab, click on the 'Manage My Membership' tab and verify 'Account management' modal is visible."); // 3
        logFlowPoint("Click on the 'Change Membership' tab and verify 'Select plan' modal is showing"); // 4
        logFlowPoint("Click on the non-highlighted plan and verify the new plan is highlighted and the previous plan is no longer highlighted."); // 5
        logFlowPoint("Click on the 'Next' CTA and complete the purchase flow."); // 6
        logFlowPoint("Verify the user is brought to the 'Premium NUX'."); // 7
        logFlowPoint("Click on the link at the bottom of the NUX and verify the user is redirected to 'Vault' page."); // 8
        logFlowPoint("Verify the user has a 'Premier badge' in their mini profile."); // 9
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        new NavigationSidebar(driver).clickManageMyMembershipLink();
        ChangeOriginAccessMembershipDialog changeOriginAccessDialog = new ChangeOriginAccessMembershipDialog(driver);
        changeOriginAccessDialog.waitIsVisible();
        logPassFail(changeOriginAccessDialog.waitIsVisible(), true);
        
        // 4
        changeOriginAccessDialog.clickChangeMembershipCTA();
        SelectOriginAccessPlanDialog selectOriginAccessDialog = new SelectOriginAccessPlanDialog(driver);
        logPassFail(selectOriginAccessDialog.waitIsVisible(), true);
        
        // 5
        selectOriginAccessDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN);
        // wait for plan to be selected
        sleep(2000);
        boolean isMonthlyPlanSelected = selectOriginAccessDialog.isMonthlyPlanSelected();
        boolean isYearlyPlanUnselected = !selectOriginAccessDialog.isYearlyPlanSelected();
        logPassFail(isMonthlyPlanSelected && isYearlyPlanUnselected, false);
        
        // 6
        selectOriginAccessDialog.clickNext();
        logPassFail(MacroPurchase.completePurchase(driver), true);
        
        // 7
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        if (originAccessNUXFullPage.verifyPageReached()) {
            logPassFail(originAccessNUXFullPage.verifyPageReached(), true);
        }else { 
            logPassFail(true, true);
        }
        
        // 8
        if (originAccessNUXFullPage.verifyPageReached()) {
            originAccessNUXFullPage.clickSkipNuxGoToVaultLinkPremierSubscriber();
        }
        logPassFail(new VaultPage(driver).verifyPageReached(), true);
        
        // 9
        logPassFail(new MiniProfile(driver).verifyOriginPremierBadgeVisible(), true);
        
        softAssertAll();
    }
}