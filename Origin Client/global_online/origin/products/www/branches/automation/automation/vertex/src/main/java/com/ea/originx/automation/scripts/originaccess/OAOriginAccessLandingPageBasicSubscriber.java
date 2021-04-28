package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
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
 * Tests 'Origin Access Landing Page' sections
 *
 * @author cdeaconu
 */
public class OAOriginAccessLandingPageBasicSubscriber extends EAXVxTestTemplate{
    
    @TestRail(caseId = 2997375)
    @Test(groups = {"originaccess", "full_regression"})
    public void testOriginAccessLandingPageBasicSubscriber (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        
        logFlowPoint("Login basic subscriber account."); // 1
        logFlowPoint("Click on 'Learn About Premier' option and verify the user navigates to the 'Premier landing' page."); // 2
        logFlowPoint("Verify the hero component is configured with an 'Upgrade CTA'."); // 3
        logFlowPoint("Verify that there is a comparison table explaining the difference between the 'Standard' and 'Premier' tiers."); // 4
        logFlowPoint("Verify there is navigation below the 'Comparison Table'."); // 5
        logFlowPoint("Verify the navigation bar displays the 'Upgarde CTA' on the nav bar on scrolling down the page."); // 6
        logFlowPoint("Verify there is a 'Premier PC' games section listing the games included with 'Premier Subscription'."); // 7
        logFlowPoint("Verify there is a list of basic and premier vault tiles listed."); // 8
        logFlowPoint("Verify there is a section explaining the upgrading of tiers."); // 9
        logFlowPoint("Verify there is a section explaining the 'Play First' benefits of 'Premier Subscription'."); // 10
        logFlowPoint("Verify there is a FAQ's section."); // 11
        logFlowPoint("Verify that there is a legal section in the footer of the page."); // 12
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        new NavigationSidebar(driver).clickLearnAboutPremierLink();
        OriginAccessPage oaPage = new OriginAccessPage(driver);
        logPassFail(oaPage.verifyPageReached(), true);
        
        // 3
        logPassFail(oaPage.verifyHeroJoinPremierButtonVisible(), false);
        
        // 4
        logPassFail(oaPage.verifyComparePlanSection(), false);
        
        // 5
        logPassFail(oaPage.verifyNavigationbarExists() && oaPage.verifyNavigationbarVisibleBelowComparisonTable(), true);
        
        // 6
        logPassFail(oaPage.verifyNavigationbarJoinPremierButtonVisible(), false);
        
        // 7
        logPassFail(oaPage.verifyPremierGamesSectionVisible() && oaPage.verifyPremierGamesListVisible(), false);
        
        // 8
        logPassFail(oaPage.verifyBasicAndPremierGamesSectionVisible() && oaPage.verifyBasicAndPremierVaultGamesListVisible(), false);
        
        // 9
        logPassFail(oaPage.verifyUpgradingToPremierSectionVisible(), false);
        
        // 10
        logPassFail(oaPage.verifyPlayFirstSectionVisible(), false);
        
        // 11
        logPassFail(oaPage.verifyFAQSectionVisible(), false);
        
        // 12
        logPassFail(oaPage.verifyLegalSectionVisible(), false);
        
        softAssertAll();
    }
}