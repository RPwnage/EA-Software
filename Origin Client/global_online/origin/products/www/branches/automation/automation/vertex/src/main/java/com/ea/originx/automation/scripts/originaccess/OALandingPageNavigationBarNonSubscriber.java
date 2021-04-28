package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests functionality of Navigation bar within 'Origin Access' landing page
 *
 * @author cbalea
 */
public class OALandingPageNavigationBarNonSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3286650)
    @Test(groups = {"origin_access", "full_regression"})
    public void testLandingPageNavigationBarNonSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_EXPIRED);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        logFlowPoint("Log into Origin with a non-subscriber account and navigate to 'Origin Access' page"); // 1
        logFlowPoint("Verify that there is navigator bar below the subscription comparison table"); // 2
        logFlowPoint("Verify that the tabs on the navigation bar scroll to the respective sections when clicked"); // 3
        logFlowPoint("Verify that below the navigation bar items there is an orange line"); // 4
        logFlowPoint("Verify upon scrolling the screen the navigation bar sticks to the top of the landing page"); // 5
        logFlowPoint("Verify that the navigation bar shows the comparison table CTA's upon scrolling down the page"); // 6

        // 1
        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        if (isClient) {
            new MainMenu(driver).clickMaximizeButton();
        }
        OriginAccessPage originAccessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        logPassFail(originAccessPage.verifyPageReached(), true);

        // 2
        logPassFail(originAccessPage.verifyNavigationbarExists(), true);

        // 3
        originAccessPage.clickComparePlanTab();
        boolean isComparisonTableDisplayed = originAccessPage.verifyComparePlanSection();
        originAccessPage.clickGamesInEAAccessTab();
        boolean isGamesInEAAccessSectionDisplayed = originAccessPage.verifyGamesInEAAccessSection();
        boolean isNavigatorTabScroll = isGamesInEAAccessSectionDisplayed && isComparisonTableDisplayed;
        logPassFail(isNavigatorTabScroll, false);

        // 4
        logPassFail(originAccessPage.verifyOrangeLineNavigationBar(), false);

        // 5
        logPassFail(originAccessPage.verifyNavigationBarSticksToTop(), false);

        // 6
        logPassFail(originAccessPage.verifyComparisonTableCTANavigationBar(), true);

        softAssertAll();
    }
}