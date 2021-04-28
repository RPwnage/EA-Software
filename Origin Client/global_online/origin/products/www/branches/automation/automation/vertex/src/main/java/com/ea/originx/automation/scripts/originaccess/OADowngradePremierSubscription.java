package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.ChangeOriginAccessMembershipDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.ReviewChangesMembershipDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.UpdatedMembershipConfirmationDialog;
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
 * Tests changing the membership plan by downgrading 
 * 
 * @author mdobre
 */
public class OADowngradePremierSubscription extends EAXVxTestTemplate {

    @TestRail(caseId = 3001056)
    @Test(groups = {"originaccess", "full_regression"})
    public void testDowngradePremierSubscription(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        
        logFlowPoint("Launch and log into 'Origin' with a premier account."); // 1
        logFlowPoint("Purchase 'Origin Access Premier'."); //2
        logFlowPoint("Hover over the Origin Access tab, click on the 'Manage My Membership' tab and verify the user is brought to the account management modal."); //3
        logFlowPoint("Click on the 'Change Membership' tab and verify the user is brought to the SKU selection modal"); //4
        logFlowPoint("Click on both SKU's and verify the SKU's are highlighted correctly."); //5
        logFlowPoint("Click on the 'Next' CTA and verify the user is brought to the 'Review Changes' modal."); //6
        logFlowPoint("Click on the 'Confirm' CTA and verify the user is brought to the 'Confirmation' tab."); //7

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        logPassFail(MacroOriginAccess.purchaseOriginPremierAccess(driver), true);
        
        //3
        new NavigationSidebar(driver).clickManageMyMembershipLink();
        ChangeOriginAccessMembershipDialog changeOriginAccessDialog = new ChangeOriginAccessMembershipDialog(driver);
        logPassFail(changeOriginAccessDialog.waitIsVisible(), true);
        
        //4
        changeOriginAccessDialog.clickChangeMembershipCTA();
        SelectOriginAccessPlanDialog selectOriginAccessDialog = new SelectOriginAccessPlanDialog(driver);
        logPassFail(selectOriginAccessDialog.waitIsVisible(), true);
        
        //5
        selectOriginAccessDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN);
        Waits.sleep(600);
        boolean isMonthlyPlanHighlighted = selectOriginAccessDialog.isMonthlyPlanSelected();
        selectOriginAccessDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.YEARLY_PLAN);
        Waits.sleep(600);
        boolean isYearlyPlanHighlighted = selectOriginAccessDialog.isYearlyPlanSelected();
        logPassFail(isMonthlyPlanHighlighted && isYearlyPlanHighlighted, false);
        
        //6
        selectOriginAccessDialog.clickNext();
        ReviewChangesMembershipDialog reviewChangesMembershipDialog = new ReviewChangesMembershipDialog(driver);
        logPassFail(reviewChangesMembershipDialog.waitIsVisible(), true);
        
        //7
        reviewChangesMembershipDialog.clickConfirmChangeCTA();
        logPassFail(new UpdatedMembershipConfirmationDialog(driver).waitIsVisible(), true); 
        
        softAssertAll();
    }
}