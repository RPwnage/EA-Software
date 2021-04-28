package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
 * Verify that there is only a 'Edit on ea.com' CTA button when
 * viewing your own profile
 *
 * @author jdickens
 */
public class OAEditProfileLink extends EAXVxTestTemplate {
    @TestRail(caseId = 13061)
    @Test(groups = {"profile", "client_only", "full_regression"})
    public void testEditProfileLink(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String userAccountName = userAccount.getUsername();

        logFlowPoint("Log into the Origin client with a random user account"); // 1
        logFlowPoint("Click on the user Avatar and select 'View my profile' and verify that the 'My Profile' page is reached"); // 2
        logFlowPoint("Verify there is only a 'Edit on ea.com' CTA button when viewing the users' profile."); // 3

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + userAccount.getUsername());
        }

        // 2
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Verify that the 'My Profile' page is reached after clicking on the user's Avatar and click 'View My Profile'");
        } else {
            logFailExit("Failed to verify that the 'My Profile' page is reached after clicking on the user's Avatar and click 'View My Profile'");
        }

        // 3
        if (profileHeader.verifyEditOnEaCTAOnlyButtonVisible()) {
            logPass("Successfully verified that the 'Edit on ea.com' CTA is the only button visible in the profile header");
        } else {
            logFail("Failed to verify that the 'Edit on ea.com' CTA is visible is the only button visible in the profile header");
        }

        softAssertAll();
    }
}
