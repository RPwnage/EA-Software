package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Performance profiling test that navigates back and forth from the Game
 * Library page, garbage collecting after every run. This should expose any
 * memory leaks in the client from the automation statistics.
 */
public class OAPerformanceGameLibrary extends EAXVxTestTemplate {

    @Test(groups = {"gamelibrary", "performance"})
    public void testPerformanceGameLibrary(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);

        logFlowPoint("Login to Origin as " + user.getUsername()); //1
        for (int i = 0; i < 10; i++) {
            logFlowPoint("Navigate to the Application Settings Page"); //2
            logFlowPoint("Navigate to the Game Library"); //3
            logFlowPoint("Garbage Collect"); //4
        }

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        NavigationSidebar navBar = new NavigationSidebar(driver);
        MainMenu mainMenu = new MainMenu(driver);
        AppSettings appSettings = new AppSettings(driver);
        GameLibrary gameLibrary = new GameLibrary(driver);
        for (int i = 0; i < 10; i++) {
            // 2
            mainMenu.selectApplicationSettings();
            if (appSettings.verifyAppSettingsReached()) {
                logPass("Successfully navigated to the Application Settings Page.");
            } else {
                logFailExit("Could not navigate to the Application Settings Page.");
            }

            // 3
            navBar.gotoGameLibrary();
            if (gameLibrary.verifyGameLibraryPageReached()) {
                logPass("Navigate to Game Library.");
            } else {
                logFailExit("Could not navigate to Game Library.");
            }

            // 4
            if (garbageCollect(driver, context)) {
                logPass("Successfully collected the garbage.");
            } else {
                logFail("Could not collect the garbage.");
            }
        }
        client.stop();
        softAssertAll();
    }
}
