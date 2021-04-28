package com.ea.originx.automation.scripts.storehome;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOfflineMode;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test store page tab message when in offline mode
 *
 * @author svaghayenegar
 */
public class OAOfflineModeStoreTab extends EAXVxTestTemplate {

    @TestRail(caseId = 39457)
    @Test(groups = {"storehome", "client_only", "full_regression"})
    public void testOfflineModeStoreTab(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch Origin and log in with a random user account"); //1
        logFlowPoint("Click Origin Menu -> Go Offline."); //2
        logFlowPoint("Go to the Store Tab.Verify that the message 'You're offline' is shown in the Store Tab"); //3
        logFlowPoint("Click Go Online.Verify that the Store reloads when the client is returned to Online Mode"); //4

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroOfflineMode.goOffline(driver), true);

        //3
        StorePage storePage = new NavigationSidebar(driver).gotoStorePage();
        logPassFail(storePage.verifyStorePageOffline(), true);

        //4
        storePage.clickGoOnline();
        storePage.waitForStorePageToLoad();
        logPassFail(storePage.verifyStorePageOnline(), true);

        softAssertAll();
    }
}
