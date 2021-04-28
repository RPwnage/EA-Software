package com.ea.originx.automation.scripts.browsegames;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreFacet;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests results shown when using the 'Platform' filter options
 *
 * @author cbalea
 */
public class OAFacetPlatform extends EAXVxTestTemplate {

    @TestRail(caseId = 2998169)
    @Test(groups = {"full_regression", "browsegames"})
    public void testFacetPlatform(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();

        final EntitlementInfo entitlementPC = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18); // pc only entitlement
        final EntitlementInfo entitlementBrowser = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.CLUB_POGO_3MONTH_MEMBERSHIP); // browser only entitlement
        final EntitlementInfo entitlementPCMac = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD); // pc/mac entitlement
        final String entitlementPCName = entitlementPC.getName();
        final String entitlementBrowserName = entitlementBrowser.getName();
        final String entitlementPCMacName = entitlementPCMac.getName();

        logFlowPoint("Log into 'Origin'"); // 1
        logFlowPoint("Navigate to 'Browse' page"); // 2
        logFlowPoint("Under Platform facet click on 'PC Download' facet and verify PC only and PC/Mac products are shown"); // 3
        logFlowPoint("Under Platform facet click on 'Mac/PC Download' facet and verify only and PC/Mac products are shown"); // 4
        logFlowPoint("Under Platform facet click on 'Mac Download' facet and verify Mac only and PC/Mac products are shown"); // 5
        logFlowPoint("Under Platform facet click on 'Browser' facet and verify browser type only products are shown"); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        BrowseGamesPage browseGamesPage = new BrowseGamesPage(driver);
        new NavigationSidebar(driver).gotoBrowseGamesPage();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);

        // 3
        StoreFacet storeFacet = new StoreFacet(driver);
        storeFacet.clickOption("Platform", "PC Download");
        boolean isPCEntitlement = browseGamesPage.verifyGameInPage(entitlementPCName);
        boolean isPCMacEntitlement = browseGamesPage.verifyGameInPage(entitlementPCMacName);
        boolean isBrowserEntitlement = !browseGamesPage.verifyGameInPage(entitlementBrowserName);
        logPassFail(isPCEntitlement && isPCMacEntitlement && isBrowserEntitlement, false);

        // 4
        storeFacet.uncheckOption("Platform", "PC Download");
        storeFacet.clickOption("Platform", "Mac/PC Download");
        isBrowserEntitlement = !browseGamesPage.verifyGameInPage(entitlementBrowserName);
        isPCEntitlement = !browseGamesPage.verifyGameInPage(entitlementPCName);
        isPCMacEntitlement = browseGamesPage.verifyGameInPage(entitlementPCMacName);
        logPassFail(isPCEntitlement && isPCMacEntitlement && isBrowserEntitlement, false);

        // 5
        storeFacet.uncheckOption("Platform", "Mac/PC Download");
        storeFacet.clickOption("Platform", "Mac Download");
        isBrowserEntitlement = !browseGamesPage.verifyGameInPage(entitlementBrowserName);
        isPCEntitlement = !browseGamesPage.verifyGameInPage(entitlementPCName);
        isPCMacEntitlement = browseGamesPage.verifyGameInPage(entitlementPCMacName);
        logPassFail(isPCEntitlement && isPCMacEntitlement && isBrowserEntitlement, false);

        // 6
        storeFacet.uncheckOption("Platform", "Mac Download");
        storeFacet.clickOption("Platform", "Browser");
        isPCEntitlement = !browseGamesPage.verifyGameInPage(entitlementPCName);
        isPCMacEntitlement = !browseGamesPage.verifyGameInPage(entitlementPCMacName);
        isBrowserEntitlement = browseGamesPage.verifyGameInPage(entitlementBrowserName);
        logPassFail(isPCEntitlement && isPCMacEntitlement && isBrowserEntitlement, false);

        softAssertAll();
    }
}
