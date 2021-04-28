package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.common.SystemMessage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.OriginAccessService;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Global Site Stripe' messaging for expired subscription
 *
 * @author cdeaconu
 */
public class OAGlobalSiteStripeMessageExpiredSubscription extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10955)
    @Test(groups = {"originaccess", "full_regression"})
    public void testGlobalSiteStripeMessageExpiredSubscription(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount account = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Purchase 'Origin Access' and then cancel the subscription."); // 2
        logFlowPoint("Verify that the banner has a message similar to 'Your Origin Access membership has expired'"); // 3
        logFlowPoint("Verify below the expired message a link 'Renew membership' is provided for renewing subscription."); // 4
        logFlowPoint("Verify clicking on the link navigates to the subscription landing page."); // 5
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, account), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        OriginAccessService.immediateCancelSubscription(account);
        // wait for Origin page to refresh
        sleep(20000);
        
        // 3
        SystemMessage systemMessage = new SystemMessage(driver);
        logPassFail(systemMessage.verifyBannerMessageVisible(), true);
        
        // 4
        logPassFail(systemMessage.verifyRenewMembershipLinkVisible(), true);
        
        // 5
        systemMessage.clickRenewMembershipLink();
        logPassFail(new OriginAccessPage(driver).verifyPageReached(), true);
        
        softAssertAll();
    }
}