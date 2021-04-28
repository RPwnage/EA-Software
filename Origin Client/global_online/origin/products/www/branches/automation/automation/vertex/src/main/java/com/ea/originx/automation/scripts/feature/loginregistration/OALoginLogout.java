package com.ea.originx.automation.scripts.feature.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests logging in and logging out on Web and Client
 *
 * @author sbentley
 */
public class OALoginLogout extends EAXVxTestTemplate {

    public void OALoginLogout(ITestContext context, boolean web) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();

        if (web) {
            logFlowPoint("Navigate to Origin in the Web Browser");  //Web 1
            logFlowPoint("Click the \"Sign In\" button on the the sidebar");    //Web 2
            logFlowPoint("Enter the account email and password, and verify the user is able to sign in and reloads the current page."); //Web 3
        } else {
            logFlowPoint("Open Origin");    //Client 1
            logFlowPoint("Enter EA Account credentials and click the \"Sign in\" button"); //Client 2
        }
        logFlowPoint("Click on the Avatar to open left navigation menu and verify correct user is signed on");   //Web 4, Client 3
        logFlowPoint("Click on the \"Sign Out\" button, and verify the user is brought to the store page and the user is not logged in"); //Web 5, Client 4

        //1
        WebDriver driver = startClientObject(context, client);

        if (web) {
            if (Waits.pollingWait(() -> new StorePage(driver).verifyStorePageReached())) {
                logPass("Successfully reached Origin");
            } else {
                logFailExit("Could not reach Origin");
            }
        } else {
            if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 30)) {
                logPass("Successfully opened Origin");
            } else {
                logFailExit("Could not open Origin");
            }
        }

        if (web) {

            //Web 2
            String mainWindowHandle = driver.getWindowHandle();

            //Page before login
            String correctPage = driver.getCurrentUrl();

            LoginPage loginPage = new LoginPage(driver);
            loginPage.clickBrowserLoginBtn();
            if (loginPage.isOnLoginPage()) {
                logPass("Successfully clicked on login button");
            } else {
                logFailExit("Could not click on the login button");
            }

            loginPage.inputUsername(userAccount.getEmail());
            loginPage.inputPassword(userAccount.getPassword());
            loginPage.clickLoginBtn();

            //Switch focus back to main window
            driver.switchTo().window(mainWindowHandle);

            boolean pageVerification = driver.getCurrentUrl().equals(correctPage);

            //Web 3
            if (MacroLogin.verifyMiniProfileUsername(driver, userAccount.getUsername()) && pageVerification) {
                logPass("Successfully logged into account " + userAccount.getUsername() + " and correct page reached.");
            } else {
                logFailExit("Could not log into account " + userAccount.getUsername() + " and could not verify correct page reached.");
            }

        } else {

            //Client 2
            MacroLogin.startLogin(driver, userAccount);

            if (MacroLogin.verifyMiniProfileUsername(driver, userAccount.getUsername())) {
                logPass("Successfully logged into account " + userAccount.getUsername());
            } else {
                logFailExit("Could not log into account " + userAccount.getUsername());
            }
        }

        //Web 4, Client 3
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.hoverOverMiniProfile();
        if (!miniProfile.isPopoutHidden()) {
            logPass("Sucessfully clicked on Avatar");
        } else {
            logFailExit("Failed clicking on Avatar");
        }

        //Web 5, Client 4
        miniProfile.selectSignOut();
        boolean signedOut = false;

        if (web) {
            signedOut = new StorePage(driver).verifyStorePageReached() && !miniProfile.verifyAvatarVisible();
        } else {
            signedOut = Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 30);
        }

        if (signedOut) {
            logPass("Successfully logged out");
        } else {
            logFail("Failed to log out");
        }

        softAssertAll();
    }
}
