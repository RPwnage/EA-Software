package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.pageobjects.store.OriginAccessFaqPage;
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
 * Test a subscriber account navigate to 'Origin Access FAQ' page, expand the
 * 'How do I join Access' link and click the 'Sign up' link redirect the user to
 * 'Vault' page
 *
 * @author cdeaconu
 */
public class OAPurchasePreventionSubscriberAccount extends EAXVxTestTemplate{
    
    @TestRail(caseId = 561397)
    @Test(groups = {"originaccess", "full_regression"})
    public void testPurchasePreventionSubscriberAccount(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
                
        logFlowPoint("Login to Origin with a subscriber account."); // 1
        logFlowPoint("Navigate to the 'Origin Access FAQ' page and expand the 'How do I join Access'."); // 2
        logFlowPoint("Click the 'Sign up' link and verify the player is taken to the 'Vault page' and there are not offers to sign up for origin access."); // 3
        logFlowPoint("Click the 'Origin Access tab' in the left nav and verify the player is taken to the 'Vault page' and there are not offers to sign up for origin access."); // 4
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        OriginAccessFaqPage originAccessFaqPage = navigationSidebar.gotoOriginAccessFaqPage();
        logPassFail(originAccessFaqPage.verifyFaqTitleMessage(), true);
        originAccessFaqPage.clickHowDoIJoinOALink();
        
        // 3
        originAccessFaqPage.clickHowDoIJoinOASignUpLink();
        VaultPage vaultPage = new VaultPage(driver);
        logPassFail(vaultPage.verifyPageReached(), true);
        
        // 4
        navigationSidebar.clickOriginAccessLink();
        logPassFail(vaultPage.verifyPageReached(), true);
        
        softAssertAll();
    }
}