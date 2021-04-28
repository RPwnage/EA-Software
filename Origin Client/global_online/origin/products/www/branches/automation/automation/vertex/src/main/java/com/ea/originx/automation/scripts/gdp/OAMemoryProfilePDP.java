package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Memory profiling test that repeatedly navigates between the 'Application Settings' page
 * and a 'PDP' page in order to expose memory leaks
 *
 * NEEDS UPDATE TO GDP
 *
 * @author jdickens
 */

public class OAMemoryProfilePDP extends EAXVxTestTemplate {

    @Test(groups = {"pdp", "memory_profile"})
    public void testMemoryProfilePDP(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);
        user.cleanFriends();
        UserAccount friendAccountA = AccountManager.getRandomAccount();
        UserAccount friendAccountB = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(user, friendAccountA, friendAccountB);

        logFlowPoint("Login to Origin with a performance account"); // 1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Navigate to the 'Application Settings' page and then a 'PDP' page, and perform garbage collection"); // 2 - 16
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
        final EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        boolean isAppSettingsPageReached = false;
        boolean isPDPPageReached = false;
        boolean isGarbageCollectionSuccessful = false;

        for (int i = 0; i < 15; i++) {
            // Select application settings from the 'Main Menu'
            mainMenu.selectApplicationSettings();
            isAppSettingsPageReached = appSettings.verifyAppSettingsReached();

            // Navigate to a 'PDP'
            navBar.gotoStorePage();
            // MacroPDP.loadPdpPage(driver, entitlementInfo);
           // PDPPage pdpPage = new PDPPage(driver);
            //  isPDPPageReached = pdpPage.verifyPDPPageReached();

            // Perform garbage collection
            isGarbageCollectionSuccessful = garbageCollect(driver, context);

            if (isAppSettingsPageReached && isPDPPageReached && isGarbageCollectionSuccessful) {
                logPass("Successfully navigated to the 'Application Settings' page followed by a 'PDP' page "
                        + "and then performed garbage collection");
            } else {
                logFail("Failed to navigate to the 'Application Settings' page followed by a 'PDP' page "
                        + "or perform garbage collection");
            }
        }

        softAssertAll();
    }
}
