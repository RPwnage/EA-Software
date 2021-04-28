package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FirewallUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies that an user can log into Origin with email, ID and in offline mode
 * 
 * @author cdeaconu
 */
public class OALoginNamesClient extends EAXVxTestTemplate{
    
    @TestRail(caseId = 1016677)
    @Test(groups = {"loginregistration", "loginregistration_smoke", "client_only",})
    public void testLoginNamesClient(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();
        final String userEmail = userAccount.getEmail();
        final String password = userAccount.getPassword();
        
        logFlowPoint("Launch Origin and log in using the user email"); // 1
        logFlowPoint("Log out of that account"); // 2
        logFlowPoint("Log in to account using the user ID"); // 3
        logFlowPoint("Log out of that account"); // 4
        logFlowPoint("Disable internet on your computer"); // 5
        logFlowPoint("Log in to the same account in offline mode"); // 6
        
        // 1
        WebDriver driver = startClientObject(context, client);
        new LoginPage(driver).quickLogin(userEmail, password);
        logPassFail(Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL, 15), true);
        
        // 2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectSignOut();
        logPassFail(Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 15), true);
        
        // 3
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 4
        miniProfile.selectSignOut();
        logPassFail(Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 15), true);
        
        // 5
        logPassFail(FirewallUtil.blockOrigin(client), true);
        
        // 6
        logPassFail(MacroLogin.networkOfflineClientLogin(driver, userAccount), true);
    }
}
