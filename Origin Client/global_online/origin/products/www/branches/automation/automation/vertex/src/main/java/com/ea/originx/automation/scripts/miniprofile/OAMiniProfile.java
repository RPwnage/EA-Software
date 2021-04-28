package com.ea.originx.automation.scripts.miniprofile;

import com.ea.vx.originclient.account.*;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;

import java.util.Arrays;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify the Mini Profile (bottom left corner)
 *
 * @author palui
 */
public class OAMiniProfile extends EAXVxTestTemplate {

    @TestRail(caseId = 1397952)
    @Test(groups = {"miniprofile", "full_regression"})
    public void testMiniProfile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        Criteria criteria = new Criteria.CriteriaBuilder().build();
        UserAccount userAccount = AccountManager.getInstance().requestWithCriteria(criteria, 60);

        logFlowPoint("Launch Origin and login as user: " + userAccount.getUsername() + ". Verify user's Origin Id appears at the 'Mini Profile'"); //1
        logFlowPoint("Verify user's Avatar appears at the 'Mini Profile'"); //2
        logFlowPoint("Verify 'Mini Profile' dropdown has menu items 'View My Profile', 'EA Account and Billing', and 'Sign Out'"); //3
        logFlowPoint("Click 'View My Profile' at the 'Mini Profile' dropdown and verify 'Profile' page opens"); //4

//@TODO: Add these steps when status on the mini profile has been implemented
//        logFlowPoint("Verify user's 'Online' status is indicated at the 'Mini Profile'"); //5
//        logFlowPoint("From Origin > Friends menu, set status to 'Away'. Verify  user's 'Away' status is indicated at the 'Mini Profile'"); //6
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
        boolean itemsExist = miniProfile.verifyPopoutMenuItemsExist(
                Arrays.asList("View My Profile", "EA Account and Billing", "Sign Out"),
                new boolean[]{true, true, true});
        if (itemsExist) {
            logPass("Verified 'Mini Profile' dropdown has menu items 'View My Profile', 'EA Account and Billing', and 'Sign Out'");
        } else {
            logFail("Failed: 'Mini Profile' dropdown missing at least one of 'View My Profile', 'EA Account and Billing', and 'Sign Out'");
        }

        //4
        miniProfile.selectProfileDropdownMenuItem("View My Profile");
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Verified 'Profile' page opens");
        } else {
            logFailExit("Failed: Cannot open the 'Profile' page through the 'Mini Profile' menu");
        }
        softAssertAll();
    }
}
