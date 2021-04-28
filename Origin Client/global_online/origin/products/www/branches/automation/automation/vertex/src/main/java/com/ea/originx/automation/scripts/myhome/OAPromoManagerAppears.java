package com.ea.originx.automation.scripts.myhome;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

import java.util.List;

/**
 * Test if there is any empty box/section in Home page and Store page by
 * checking existence of image
 *
 * @author rocky
 */
public class OAPromoManagerAppears extends EAXVxTestTemplate {

    @TestRail(caseId = 1016721)
    @Test(groups = {"myhome", "services_smoke", "browser_only"})
    public void testPromoManagerAppears(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String userName = userAccount.getUsername();

        logFlowPoint("Login to Origin"); //1
        logFlowPoint("Navigate to Store page and verify if there is any empty section/box by checking for existence of image"); //2
        logFlowPoint("Navigate to Home page, clicking view more buttons and verify if there is any empty section/box by checking for existence of image"); //3

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with newly registered user " + userName);
        } else {
            logFailExit("Could not log into Origin with newly registered user." + userName);
        }

        // 2
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.gotoStorePage();
        StorePage storePage = new StorePage(driver);
        storePage.waitForPageToLoad();
        List<String> storeImageNotFoundList = storePage.verifyAllImagesExist();
        if (storeImageNotFoundList.isEmpty()) {
            logPass("Verified there is no empty box/section due to missing image in Store page");
        } else {
            logFail("Failed: found one or more missing image " + storeImageNotFoundList + " in Store page");
        }

        // 3
        navigationSidebar.gotoDiscoverPage();
        DiscoverPage discoverPage = new DiscoverPage(driver);
        discoverPage.waitForPageToLoad();
        discoverPage.clickViewMoreButtons();
        discoverPage.waitForPageToLoad();
        // still showing view more button
        discoverPage.clickViewMoreButtons();
        List<String> homeImageNotFoundList = discoverPage.verifyAllImagesExist();
        if (homeImageNotFoundList.isEmpty()) {
            logPass("Verified there is no empty box/section due to missing image in Home page");
        } else {
            logFail("Failed: found one or more missing image " + homeImageNotFoundList + " in Home page");
        }

        softAssertAll();
    }
}
