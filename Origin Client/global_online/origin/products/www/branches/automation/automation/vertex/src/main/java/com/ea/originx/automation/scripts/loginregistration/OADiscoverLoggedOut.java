package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test 'Discover' link and 'Discover' page disappearing after logout
 *
 * @author palui
 */
public class OADiscoverLoggedOut extends EAXVxTestTemplate {

    @TestRail(caseId = 9505)
    @Test(groups = {"loginregistration", "browser_only"})
    public void testDiscoverLoggedOut(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        String userName = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as user: " + userName); //1
        logFlowPoint("Verify 'Discover' link at the 'Navigation Sidebar' is visible"); //2
        logFlowPoint("Navigate to 'Discover' page"); //3
        logFlowPoint("Logout. Verify 'Discover' link at the 'Navigation Sidebar' is no longer visible"); //4
        logFlowPoint("Verify user is no longer at the 'Discover' page"); //5

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + userName);
        } else {
            logFailExit("Failed: Cannot login as user: " + userName);
        }

        //2
        if (new NavigationSidebar(driver).verifyDiscoverLinkVisible()) {
            logPass("Verified 'Discover' link is visible");
        } else {
            logFailExit("Failed: 'Discover' link is not visible");
        }

        //3
        DiscoverPage discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen(); // close TakeOver page if opened
        if (discoverPage.verifyDiscoverPageReached()) {
            logPass("Verified  'Discover' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Discover' page");
        }

        //4
        new MiniProfile(driver).selectSignOut();
        NavigationSidebar navSidebar = new NavigationSidebar(driver);
        boolean discoverLinkAbsent = Waits.pollingWait(() -> !navSidebar.verifyDiscoverLinkVisible());
        if (discoverLinkAbsent) {
            logPass("Verified 'Discover' link is no longer visible after logout");
        } else {
            logFailExit("Failed: 'Discover' link is still visible after logout");
        }

        //5
        if (!discoverPage.verifyDiscoverPageReached()) {
            logPass("Verified user is no longer at the 'Discover' page after logout");
        } else {
            logFailExit("Failed: User is still at the 'Discover' page after logout");
        }

        softAssertAll();
    }

}
