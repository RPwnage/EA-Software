package com.ea.originx.automation.scripts.myhome;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Memory profiling test that repeatedly navigates between the 'Application Settings' page
 * and the 'My Home' page in order to expose memory leaks
 *
 * @author jdickens
 */

public class OAMemoryProfileMyHome extends EAXVxTestTemplate {

    @Test(groups = {"myhome", "memory_profile"})
    public void testMemoryProfileMyHome(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);
        user.cleanFriends();
        UserAccount friendAccountA = AccountManager.getRandomAccount();
        UserAccount friendAccountB = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(user, friendAccountA, friendAccountB);

        logFlowPoint("Login to Origin with a performance account"); // 1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Navigate to the 'Application Settings' page and then the 'My Home' page, and finally collect the garbage"); // 2 - 16
        }

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with user " + user.getUsername());
        } else {
            logFailExit("Failed to log into Origin with the user " + user.getUsername());
        }

        // 2 - 16
        NavigationSidebar navBar = new NavigationSidebar(driver);
        MainMenu mainMenu = new MainMenu(driver);
        AppSettings appSettings = new AppSettings(driver);
        DiscoverPage discoverPage = new DiscoverPage(driver);
        boolean isAppSettingsPageReached = false;
        boolean isMyHomePageReached = false;
        boolean isGarbageCollectionSuccessful = false;

        for (int i = 0; i < 15; i++) {
            // Select 'Application Settings' from the 'Main Menu'
            mainMenu.selectApplicationSettings();
            isAppSettingsPageReached = appSettings.verifyAppSettingsReached();

            // Select the 'My Home' page from the 'Navigation Bar'
            navBar.gotoDiscoverPage();
            isMyHomePageReached = discoverPage.verifyDiscoverPageReached();

            // Collect the garbage
            isGarbageCollectionSuccessful = garbageCollect(driver, context);

            if (isAppSettingsPageReached && isMyHomePageReached && isGarbageCollectionSuccessful) {
                logPass("Successfully navigated to the 'Application Settings' page followed by the 'My Home' page "
                        + "and then performed the garbage collection");
            } else {
                logFail("Failed to navigate to the 'Application Settings' page followed by the 'My Home' page "
                        + "or perform garbage collection");
            }
        }

        softAssertAll();
    }
}