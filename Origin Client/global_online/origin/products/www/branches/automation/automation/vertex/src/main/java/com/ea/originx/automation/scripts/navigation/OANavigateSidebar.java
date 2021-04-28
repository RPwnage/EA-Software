package com.ea.originx.automation.scripts.navigation;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the basic navigation functions of the sidebar
 *
 * @author rmcneil
 */
public class OANavigateSidebar extends EAXVxTestTemplate {

    @TestRail(caseId = 1016682)
    @Test(groups = {"navigation", "full_regression", "release_smoke", "long_smoke"})
    public void testNavigateSidebar(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount user;
        final String environment = OSInfo.getEnvironment();
        if(environment.equalsIgnoreCase("production")) {
            user = AccountManager.getInstance().createNewThrowAwayAccount();
        } else {
            user = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        }

        logFlowPoint("Log into Origin with " + user.getUsername()); // 1
        logFlowPoint("Navigate to Store Main Page"); // 2
        logFlowPoint("Navigate to Store > Browse Games"); // 3
        logFlowPoint("Navigate to Store > Deals"); // 4
        logFlowPoint("Navigate to Origin Access"); // 5
        logFlowPoint("Navigate to Origin Access > Vault Games"); // 6
        logFlowPoint("Navigate to Origin Access > Play First Trials"); // 7
        logFlowPoint("Navigate to Origin Access > Origin Access FAQ"); // 8
        logFlowPoint("Navigate to My Home"); // 9
        logFlowPoint("Navigate to Game Library"); // 10

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        // 2
        if(environment.equalsIgnoreCase("production")) {
            new MainMenu(driver).selectRedeemProductCode();
            MacroRedeem.redeemOABasicSubscription(driver, user.getEmail());
        }
        NavigationSidebar navBar = new NavigationSidebar(driver);
        StorePage storePage = navBar.gotoStorePage();
        logPassFail(storePage.verifyStorePageReached(), false);

        // 3
        BrowseGamesPage browseGamesPage = navBar.gotoBrowseGamesPage();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), false);

        // 4
        navBar.clickDealsExclusivesLink();
        logPassFail(Waits.pollingWait(navBar::verifyDealsExclusivesActive), false);

        // 5
        navBar.clickOriginAccessLink();
        logPassFail(Waits.pollingWait(navBar::verifyOriginAccessActive), false);

        // 6
        navBar.gotoVaultGamesPage();
        logPassFail(Waits.pollingWait(navBar::verifyOriginAccessActive), false);

        // 7
        navBar.gotoOriginAccessTrialPage();
        logPassFail(Waits.pollingWait(navBar::verifyOriginAccessActive, 3000, 200, 2000), false);

        // 8
        navBar.gotoOriginAccessFaqPage();
        logPassFail(Waits.pollingWait(navBar::verifyOriginAccessActive), false);

        // 9
        DiscoverPage discoverPage = navBar.gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        logPassFail(Waits.pollingWait(discoverPage::verifyDiscoverPageReached, 3000, 500, 1000), false);

        // 10
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(Waits.pollingWait(gameLibrary::verifyGameLibraryPageReached, 3000, 500, 1000), false);

        softAssertAll();
    }
}