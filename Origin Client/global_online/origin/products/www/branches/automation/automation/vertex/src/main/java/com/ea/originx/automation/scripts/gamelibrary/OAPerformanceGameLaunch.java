package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSettings;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Performance profiling test that launches, exits DIP Small repeatedly, garbage collecting after every run.
 * This should expose possible memory leaks in the client from the automation statistics.
 *
 * @author micwang
 */
public class OAPerformanceGameLaunch extends EAXVxTestTemplate {

    @Test(groups = {"gamelibrary", "performance"})
    public void testPerformanceGameLaunch(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final OADipSmallGame entitlement = new OADipSmallGame();
        final UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);

        logFlowPoint("Login to Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to the Game Library"); //2
        logFlowPoint("Download and install DiP Small"); //3
        for (int i = 0; i < 10; i++) {
            logFlowPoint("Launch and exit DiP Small"); //4, 6, 8..., 22
            logFlowPoint("Garbage Collect"); //5, 7, 9..., 23
        }

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        // Two reason to turn off cloud save here:
        // 1. this test case used peggle originally, which has no cloud save
        // 2. test run is more fragile when having cloud save on
        MacroSettings.changeCloudSaveSettings(driver, false);
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(Waits.pollingWait(gameLibrary::verifyGameLibraryPageReached), true);

        //3
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlement.getOfferId()), true);
        for (int i = 0; i < 10; i++) {
            //4, 6, 8..., 22
            logPassFail(MacroGameLibrary.launchByJsSDKAndExitGame(driver, entitlement, client), true);

            //5, 7, 9..., 23
            logPassFail(garbageCollect(driver, context), false);
        }
        client.stop();
        softAssertAll();
    }
}
