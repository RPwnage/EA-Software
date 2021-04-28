package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test an underage user is not able to log into a browser.
 *
 * @author RCHOI
 */
public class OAUnderageLoginWeb extends EAXVxTestTemplate {

    @TestRail(caseId = 1016681)
    @Test(groups = {"loginregistration", "release_smoke", "browser_only","release_smoke"})
    public void testOAUnderageLoginWeb(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // generate random adult email account which will be used for parentalEmail for underage user account
        // This random account won't be registered to origin
        String randomAdultEmailAddress = AccountManagerHelper.createNewThrowAwayAccount(30).getEmail();
        // adding random adult email address to underage user account creation.
        UserAccount userAccount = AccountManagerHelper.registerNewUnderAgeThrowAwayAccountThroughREST(10, randomAdultEmailAddress);
        String underageUserID = userAccount.getUsername();
        String underageUserPassword = userAccount.getPassword();

        logFlowPoint("Verify underage user is unable to login at a browser using the underage user's ID " + underageUserID);
        logFlowPoint("Verify underage user is unable to login at a browser using the underage user's parent email address  " + randomAdultEmailAddress);

        //1
        // check underage user login using user ID
        WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.enterInformation(underageUserID, underageUserPassword);
        Waits.pollingWaitEx(loginPage::isOnLoginPage, 2000, 500, 0);
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Verified underage user is unable to login at a browser using the underage user's ID " + underageUserID);
        } else {
            logFailExit("Failed: underage user is Able to login at a browser using the underage user's ID " + underageUserID);
        }

        //2
        // check underage user login using parent's email address
        loginPage.enterInformation(randomAdultEmailAddress, underageUserPassword);
        Waits.pollingWaitEx(loginPage::isOnLoginPage, 2000, 500, 0);
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Verified underage user is unable to login at a browser using the underage user's parent email address  " + randomAdultEmailAddress);
        } else {
            logFailExit("Failed: underage user is Able to login at a browser using the underage user's parent email address  " + randomAdultEmailAddress);
        }
        softAssertAll();
    }
}
