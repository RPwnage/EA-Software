package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests upgrading to premier subscription dialog will show up when a basic
 * subscriber account clicks all the upgrades to 'Premier' CTAs on the 'Origin
 * Access' page
 *
 * @author cdeaconu
 */
public class OAStandardSubscriberUpgradingToPremier extends EAXVxTestTemplate{
    
    @TestRail(caseId = 2997375)
    @Test(groups = {"originaccess", "full_regression"})
    public void testStandardSubscriberUpgradingToPremier (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        
        logFlowPoint("Login with a basic subscriber account."); // 1
        logFlowPoint("Click on 'Learn About Premier' option and verify the user navigates to the 'Premier landing' page"); // 2
        logFlowPoint("Click on the 'Join Premier' CTA on the hero and verify that the upgrading to premier flow shows up."); // 3
        logFlowPoint("Close the 'SKU Modal'."); // 4
        logFlowPoint("Click on the 'Join Premier' CTA from the 'Compare table' and verify that the upgrading to premier flow shows up."); // 5
        logFlowPoint("Close the 'SKU Modal'."); // 6
        logFlowPoint("Scroll down on the landing page until the navigation bar shows an upgrade CTA."); // 7
        logFlowPoint("Click on the 'Join Premier' CTA from the navigation bar and verify that the upgrading to premier flow shows up."); // 8
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        new NavigationSidebar(driver).clickLearnAboutPremierLink();
        OriginAccessPage oaPage = new OriginAccessPage(driver);
        logPassFail(oaPage.verifyPageReached(), true);
        
        // 3
        oaPage.clickJoinPremierButton();
        SelectOriginAccessPlanDialog selectOriginAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        logPassFail(selectOriginAccessPlanDialog.waitIsVisible(), true);
        
        // 4
        selectOriginAccessPlanDialog.closeAndWait();
        logPassFail(selectOriginAccessPlanDialog.waitIsClose(), true);
        
        // 5
        oaPage.clickComparisonTableJoinPremierButton();
        logPassFail(selectOriginAccessPlanDialog.waitIsVisible(), true);
        
        // 6
        selectOriginAccessPlanDialog.closeAndWait();
        logPassFail(selectOriginAccessPlanDialog.waitIsClose(), true);
        
        // 7
        logPassFail(oaPage.verifyNavigationbarJoinPremierButtonVisible(), true);
        
        // 8
        oaPage.clickNavigationBarJoinPremierButton();
        logPassFail(selectOriginAccessPlanDialog.waitIsVisible(), true);
        
        softAssertAll();
    }
}