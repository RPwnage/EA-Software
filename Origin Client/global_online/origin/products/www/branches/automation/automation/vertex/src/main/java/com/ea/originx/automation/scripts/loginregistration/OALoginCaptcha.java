package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test login captcha by logging in with invalid credentials multiple times
 *
 * @author palui
 */
public class OALoginCaptcha extends EAXVxTestTemplate {

    @TestRail(caseId = 9477)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testLoginCaptcha(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String username = userAccount.getUsername();

        final int timesToTriggerCaptcha = MacroLogin.NUMBER_OF_INVALID_LOGINS_TO_TRIGGER_CAPTCHA;

        logFlowPoint(String.format("Launch Origin and login with a wrong password %d times for Login Id: %s", timesToTriggerCaptcha, username));//1
        logFlowPoint("Verify captcha appears");//2
        logFlowPoint("Enter login Id, the correct password and the captcha. Verify login successful for: " + username);//3
        logFlowPoint("Logout from Origin");//4
        // Skip network offline testing for browser tests
        if (isClient) {
            logFlowPoint(String.format("Login with a wrong password %d times again. Verify captcha re-appears", timesToTriggerCaptcha));//5
            logFlowPoint("Disconnect Origin from the network");//6
            logFlowPoint("Enter login Id, the correct password and the captcha. Verify login successful for: " + username);//7
        }

        //1
        WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = initPage(driver, context, LoginPage.class);

        if (MacroLogin.loginTriggerCaptcha(driver, userAccount)) {
            logPass(String.format("Verified login with wrong password fails %d times as expected for login Id: %s", timesToTriggerCaptcha, username));
        } else {
            logFailExit(String.format("Failed: Login with wrong password does not fail %d times as expected for login Id: %s", timesToTriggerCaptcha, username));
        }

        //2
        if (loginPage.verifyCaptchaVisible()) {
            logPass("Verified captcha appears at the Login window");
        } else {
            logFailExit("Failed: Captcha does not appear at the Login window");
        }

        //3
        if (MacroLogin.loginWithCaptcha(driver, userAccount)) {
            logPass("Verified login succesful with captcha entered for login Id: " + username);
        } else {
            logFailExit("Failed: Login not successful as expected for login Id: " + username);
        }

        //4
        new MiniProfile(driver).selectSignOut();
        if (!isClient) {
            loginPage.clickBrowserLoginBtn();
        }

        boolean isLoginPage = Waits.pollingWait(() -> loginPage.isPageThatMatchesOpen(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Verified logout successful and Login page re-appears");
        } else {
            logFailExit("Failed: Cannot logout from Origin or Login page does not re-appear");
        }

        // Skip the following steps for browser tests
        if (isClient) {
            //5
            MacroLogin.loginTriggerCaptcha(driver, userAccount);
            if (loginPage.verifyCaptchaVisible()) {
                logPass("Verified captcha appears at the Login window");
            } else {
                logFailExit("Failed: Captcha does not appear at the Login window");
            }

            //6
            if (FirewallUtil.blockOrigin(client)) {
                logPass("Verified Origin successfully blocked from the network");
            } else {
                logFailExit("Failed: Cannot block Origin from the network");
            }

            // Restart Origin to trigger offline mode in the login screen
            client.stop();
            driver = startClientObject(context, client);

            //7
            // Login, then exit network offline. Otherwise Origin spins and login cannot succeed
            if (MacroLogin.loginTriggerCaptcha(driver, userAccount)) {
                logPass(String.format("Verified login with captcha while Origin is disconnected is succesful for login Id: %s", username));
            } else {
                logFailExit(String.format("Failed: Login with captcha while Origin is disconnected is not successful for login Id: %s", username));
            }
        }
        softAssertAll();
    }

}
