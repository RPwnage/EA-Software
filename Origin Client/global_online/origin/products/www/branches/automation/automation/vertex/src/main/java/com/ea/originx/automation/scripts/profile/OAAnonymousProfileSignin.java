package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNavigation;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
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
 * Tests Sign in CTA allows a logged out user to sign in at Profile page
 *
 * @author alcervantes
 */
public class OAAnonymousProfileSignin extends EAXVxTestTemplate {

    @TestRail(caseId = 9419)
    @Test(groups = {"profile", "full_regression", "browser_only"})
    public void testAnonymousProfileSignin(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Add '/profile/achievements' to the end of the origin URL you are testing with.");//1
        logFlowPoint("Observe a 'Not signed in' error message with 'Sign In' and 'Register' CTA's");//2
        logFlowPoint("Click on 'Sign In' CTA and sign in to Origin");//3
        logFlowPoint("Verify that the 'Sign In' CTA allows a logged out user to sign in");//4

        //1
        WebDriver driver = startClientObject(context, client);
        MacroNavigation quickNav = new MacroNavigation(driver);
        logPassFail(quickNav.toAnonymousProfileAchievementsTab(), true);

        //2
        ProfileHeader profileHeader = new ProfileHeader(driver);
        logPassFail(profileHeader.verifyOnSignInPage(), true);

        //3
        profileHeader.clickLoginCtaBtn();
        logPassFail(MacroLogin.loginAnonymousProfile(driver, userAccount), true);

        //4
        logPassFail(profileHeader.verifyAchievementsTabActive(), true);

    }
}
