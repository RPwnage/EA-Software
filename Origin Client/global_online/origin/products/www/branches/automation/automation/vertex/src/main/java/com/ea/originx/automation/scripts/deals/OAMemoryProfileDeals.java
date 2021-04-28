package com.ea.originx.automation.scripts.deals;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.store.DealsPage;
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
 * and the 'Deals' page in order to expose memory leaks
 *
 * @author jdickens
 */

public class OAMemoryProfileDeals extends EAXVxTestTemplate {

    @Test(groups = {"deals", "memory_profile"})
    public void testMemoryProfileDealsPage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);
        user.cleanFriends();
        UserAccount friendAccountA = AccountManager.getRandomAccount();
        UserAccount friendAccountB = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(user, friendAccountA, friendAccountB);

        logFlowPoint("Login to Origin with a performance account"); // 1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Navigate to the 'Application Settings' page and then the 'Deals' page, and perform garbage collection"); // 2 - 16
        }

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with user " + user.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + user.getUsername());
        }

        // 2 - 16
        NavigationSidebar navBar = new NavigationSidebar(driver);
        MainMenu mainMenu = new MainMenu(driver);
        AppSettings appSettings = new AppSettings(driver);
        DealsPage dealsPage = new DealsPage(driver);
        boolean isAppSettingsPageReached = false;
        boolean isDealsPageReached = false;
        boolean isGarbageCollectionSuccessful = false;

        for (int i = 0; i < 15; i++) {
            // Select application settings from the 'Main Menu'
            mainMenu.selectApplicationSettings();
            isAppSettingsPageReached = appSettings.verifyAppSettingsReached();

            // Select the 'Deals' page from the nav bar
            navBar.gotoDealsPage();
            isDealsPageReached = dealsPage.verifyDealsPageReached();

            // Perform garbage collection
            isGarbageCollectionSuccessful = garbageCollect(driver, context);

            if (isAppSettingsPageReached && isDealsPageReached && isGarbageCollectionSuccessful) {
                logPass("Successfully navigated to the 'Application Settings' page followed by the 'Deals' page "
                        + "and then performed garbage collection");
            } else {
                logFail("Failed to navigate to the 'Application Settings' page followed by the 'Deals' page "
                        + "or perform garbage collection");
            }
        }

        softAssertAll();
    }
}
