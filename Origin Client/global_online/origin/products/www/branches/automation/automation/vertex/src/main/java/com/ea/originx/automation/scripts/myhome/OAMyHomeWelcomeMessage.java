package com.ea.originx.automation.scripts.myhome;
 
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.helpers.ContextHelper;
import static com.ea.vx.originclient.templates.OATestBase.sleep;
import org.openqa.selenium.WebDriver;
 
/**
 * This script tests the basics of the 'My Home' welcome message.
 * 
 * @author jdickens
 */
 
public class OAMyHomeWelcomeMessage extends EAXVxTestTemplate {
    
    @TestRail(caseId = 575199)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testMyHomeWelcomeMessage(ITestContext context) throws Exception { 
 
        OriginClient client = OriginClientFactory.create(context);
        UserAccount user = AccountManager.getRandomAccount();
        final String userName = user.getUsername();
        
        logFlowPoint("Log into Origin with a random account"); // 1
        logFlowPoint("Go to the 'My Home' page and verify that the welcome message is visible at the top of the page"); // 2
        logFlowPoint("Verify that the welcome message mentions the username"); //3
        logFlowPoint("Navigate to any other page, return to the 'my home' page, and verify that a welcome message is visible"); // 4
        logFlowPoint("Reload Origin while on the 'my home' page and verify that a welcome message is visible"); // 5
        logFlowPoint("Log out, log back in, return to 'my home', and verify that a welcome message is visible"); // 6
        
        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {    
            logPass("Successfully logged into Origin with random account");
        } else {
            logFailExit("Could not log into Origin with random account");
        }
        
        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        DiscoverPage discoverPage = navBar.gotoDiscoverPage();
        sleep(2000); // sleep sometimes necessary for slow refreshes
        discoverPage.waitForPage();
        boolean isWelcomeMessagePresent = discoverPage.verifyWelcomeMessageVisible();
        if (isWelcomeMessagePresent) {
            logPass("The welcome message is present at the top of 'My Home' page");
        } else {
            logFailExit("The welcome message is not present at the top of 'My Home' page");
        }
        
        // 3
        boolean isUserNameContainedInWelcomeMessage = discoverPage.verifyWelcomeMessageContainsUserName(userName);
        if (isUserNameContainedInWelcomeMessage) {
            logPass("The welcome message mentions the username");
        } else {
            logFailExit("The welcome does not mention the username");
        }
       
        // 4 
        StorePage storePage = navBar.gotoStorePage();
        boolean isStorePageReached = storePage.verifyStorePageReached();
        discoverPage = navBar.gotoDiscoverPage();
        sleep(2000);// sleep sometimes necessary to wait for slow refreshes
        discoverPage.waitForPage();
        isWelcomeMessagePresent = discoverPage.verifyWelcomeMessageVisible();
        if (isStorePageReached && isWelcomeMessagePresent) {
            logPass("The welcome message is visible at the top of 'My Home' page after navigating to the another page");
        } else {
            logFailExit("The welcome message is not visible at the top of 'My Home' page after navigating to another page");
        }
        
        // 5
        driver.navigate().refresh();
        sleep(5000);// sleep necessary to wait for slow refreshes
        discoverPage.waitForPage();
        isWelcomeMessagePresent = discoverPage.verifyWelcomeMessageVisible();
        if (isWelcomeMessagePresent) {
            logPass("After refreshing, the welcome message is present at the top of 'My Home' page");
        } else {
            logFailExit("After refreshing, the welcome message is not present at the top of 'My Home' page");
        }
        
        // 6
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        miniProfile.selectSignOut();
        boolean isSignedIn = MacroLogin.startLogin(driver, user);
        navBar = new NavigationSidebar(driver);
        discoverPage = navBar.gotoDiscoverPage();
        discoverPage.waitForPage();
        isWelcomeMessagePresent = discoverPage.verifyWelcomeMessageVisible();
        if (isWelcomeMessagePresent && isSignedIn) {
            logPass("After signing out and signing back in, the welcome message is present at the top of the 'My Home' page");
        } else {
            logFailExit("After signing out and signing back in, the welcome message is not present at the top of the 'My Home' page");
        } 
        
        softAssertAll();
    }
}