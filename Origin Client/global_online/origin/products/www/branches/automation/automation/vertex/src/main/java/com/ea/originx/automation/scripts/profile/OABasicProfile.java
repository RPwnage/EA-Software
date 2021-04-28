package com.ea.originx.automation.scripts.profile;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.*;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify the Mini Profile (bottom left corner)
 *
 * @author palui
 */
public class OABasicProfile extends EAXVxTestTemplate {

    @TestRail(caseId = 3121918)
    @Test(groups = {"profile", "quick_smoke", "long_smoke"})
    public void testBasicProfile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch Origin and login as user: " + userAccount.getUsername() + ". Verify user's Origin Id appears at the 'Mini Profile'"); //1
        logFlowPoint("Verify user's Avatar appears at the 'Mini Profile'"); //2
        logFlowPoint("Verify the User's Name is Correct");
        logFlowPoint("Click 'View My Profile' at the 'Mini Profile' dropdown and verify 'Profile' page opens"); //4
        logFlowPoint("Verify " + userAccount.getUsername() + " Appears on the Profile Header"); //5
        logFlowPoint("Verify the Profile Displays an Avatar"); //6
        logFlowPoint("Verify There is a Section of Navigation Tabs"); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        if (miniProfile.verifyAvatarVisible()) {
            logPass("Verified user's Avatar appears at the 'Mini Profile' ");
        } else {
            logFailExit("Failed: user's Avatar does not appear at the 'Mini Profile'");
        }

        //3
        boolean correctName = miniProfile.verifyUser(userAccount.getUsername());
        if (correctName) {
            logPass("Correct Username appears on the Mini Profile.");
        } else {
            logFail("Correct Username does not appear on the Mini Profile.");
        }

        //4
        miniProfile.selectProfileDropdownMenuItem("View My Profile");
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Verified 'Profile' page opens");
        } else {
            logFailExit("Failed: Cannot open the 'Profile' page through the 'Mini Profile' menu");
        }

        // 5
        boolean headerName = profileHeader.verifyUsername(userAccount.getUsername());
        if (headerName) {
            logPass("Correct Username appears on Profile Header.");
        } else {
            logFail("Username on the Profile Header does not match current user.");
        }

        // 6
        String avatarSrc = profileHeader.getAvatarSrc();
        if (avatarSrc != null && !avatarSrc.isEmpty()) {
            logPass("Avatar exists on the profile header.");
        } else {
            logFail("Avatar does not exist on the profile header.");
        }

        // 7
        if (profileHeader.verifyNavigationSection()) {
            logPass("Navigation tab section is visible.");
        } else {
            logFailExit("No navigation tab section is visible.");
        }

        softAssertAll();
    }
}
