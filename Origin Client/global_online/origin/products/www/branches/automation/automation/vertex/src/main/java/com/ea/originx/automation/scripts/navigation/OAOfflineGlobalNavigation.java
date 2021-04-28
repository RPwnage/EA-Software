package com.ea.originx.automation.scripts.navigation;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import static com.ea.vx.originclient.utils.Waits.waitForPageThatStartsWith;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileOfflinePage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests changes to top-level pages in 'Offline Mode' with the global navigation 'Back' button.
 *
 * @author rmcneil
 */
public class OAOfflineGlobalNavigation extends EAXVxTestTemplate {

    @TestRail(caseId = 39458)
    @Test(groups = {"navigation", "client_only", "full_regression"})
    public void testOfflineGlobalNavigation(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String smallDip = EntitlementInfoConstants.DIP_SMALL_NAME;

        AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        criteria.addEntitlement(smallDip);
        UserAccount user = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Login to Origin with " + user.getUsername()); // 1
        logFlowPoint("Navigate to the Store Page"); // 2
        logFlowPoint("Navigate to the Game Library Page"); // 3
        logFlowPoint("Navigate to the Profile Page"); // 4
        logFlowPoint("Select 'Go Offline' from the Origin Menu"); // 5
        logFlowPoint("Click back on the Global Navigation bar and verify entitlements are visible in the Game Library"); // 6
        logFlowPoint("Click back on the Global Navigation bar and verify the Store Page is in offline mode"); // 7
        logFlowPoint("Click back on the Global Navigation bar and verify the Discover Page is in offline mode"); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin.");
        } else {
            logFailExit("Could not log into Origin.");
        }

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        StorePage storePage = navBar.gotoStorePage();
        if (storePage.verifyStorePageReached()) {
            logPass("Navigated to the Store Main Page.");
        } else {
            logFail("Did not return to the Store Main Page.");
        }

        // 3
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Navigated to the Game Library Page.");
        } else {
            logFailExit("Did not return to the Game Library Page.");
        }

        // 4
        MacroSocial.restoreAndExpandFriends(driver);
        MiniProfile userMiniProfile = new MiniProfile(driver);
        userMiniProfile.clickProfileAvatar();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (Waits.pollingWait(() -> profileHeader.verifyOnProfilePage())) {
            logPass("Sucessfully navigated to the User's profile page.");
        } else {
            logFailExit("Could not navigate to the User's profile page.");
        }

        // 5
        new MainMenu(driver).selectGoOffline();
        waitForPageThatStartsWith(driver, "https", 120);

        ProfileOfflinePage profileOfflinePage = new ProfileOfflinePage(driver);
        if (profileOfflinePage.verifyOnOfflinePage()) {
            logPass("Verified Offline Profile page loaded");
        } else {
            logFailExit("Offline Profile page did not loaded");
        }

        // 6
        navBar.clickBack();
        boolean gameLibraryReached = gameLibrary.verifyGameLibraryPageReached();
        boolean gameTileVisible = gameLibrary.isGameTileVisibleByName(smallDip);
        if (gameLibraryReached && gameTileVisible) {
            logPass("Global Navigation Back button was clicked and the offline Game Library Page was opened.");
        } else {
            logFail("Global Navigation Back button was clicked but the offline Game Library Page did not open.");
        }

        // 7
        navBar.clickBack();
        if (storePage.verifyStorePageOffline()) {
            logPass("Global Navigation Back button was clicked and the offline Store Main Page opened.");
        } else {
            logFail("Global Navigation Back button was clicked and the offline Store Main Page did not open.");
        }

        // 8
        navBar.clickBack();
        DiscoverPage discoverPage = new DiscoverPage(driver);
        if (Waits.pollingWait(() -> discoverPage.verifyOfflineMessageExists())) {
            logPass("Global Navigation Back button was clicked and the offline Discover Page opened.");
        } else {
            logFail("Global Navigation Back button was clicked and the offline Discover Page did not open.");
        }

        softAssertAll();
    }
}
