package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.webdrivers.BrowserType;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the profile games tab when the user has no games.
 *
 * @author lscholte
 */
public class OAProfileNoGames extends EAXVxTestTemplate {

    @TestRail(caseId = 13094)
    @Test(groups = {"full_regression", "social"})
    public void testProfileNoGames(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);

        final AccountTags tag = AccountTags.NO_ENTITLEMENTS;
        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(tag);
        UserAccount friendAccount = AccountManagerHelper.getTaggedUserAccount(tag);

        userAccount.cleanFriends();
        friendAccount.cleanFriends();

        userAccount.addFriend(friendAccount);


        logFlowPoint("Login to Origin as " + userAccount.getUsername()); //1
        logFlowPoint("Open the profile using the mini profile menu"); //2
        logFlowPoint("Navigate to the 'Games' tab on the profile"); //3
        logFlowPoint("Verify the 'No Games' message is visible"); //4
        logFlowPoint("Navigate to the friend's profile"); //5
        logFlowPoint("Navigate to the 'Games' tab on the friend's profile"); //6
        logFlowPoint("Verify the 'No Games' message is visible on the friend's profile"); //7

        // 1
        //Disabling privacy if enabled on friend account
        WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        MacroAccount.setPrivacyThroughBrowserLogin(browserDriver, friendAccount, PrivacySettingsPage.PrivacySetting.FRIENDS);
        browserDriver.close();
        WebDriver driver = startClientObject(context, client);

        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        miniProfile.selectProfileDropdownMenuItem("View My Profile");
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Successfully opened the profile through the mini profile menu");
        } else {
            logFailExit("Could not open the profile through the mini profile menu");
        }

        //3
        profileHeader.openGamesTab();
        if (profileHeader.verifyGamesTabActive()) {
            logPass("Navigated to the 'Games' tab in the profile");
        } else {
            logFailExit("Could not navigate to the 'Games' tab in the profile");
        }

        //4
        ProfileGamesTab gamesTab = new ProfileGamesTab(driver);
        if (gamesTab.verifyNoGamesMessage(true)) {
            logPass("The 'No Games' message appears");
        } else {
            logFail("The 'No Games' message does not appear");
        }

        //5
        if (MacroProfile.navigateToFriendProfile(driver, friendAccount.getUsername())) {
            logPass("Successfully opened the friend's profile");
        } else {
            logFailExit("Could not open the friend's profile");
        }

        //6
        profileHeader.openGamesTab();
        if (profileHeader.verifyGamesTabActive()) {
            logPass("Navigated to the 'Games' tab in the friend's profile");
        } else {
            logFailExit("Could not navigate to the 'Games' tab in the friend's profile");
        }

        //7
        if (gamesTab.verifyNoGamesMessage(false)) {
            logPass("The 'No Games' message appears on the friend's profile");
        } else {
            logFail("The 'No Games' message does not appear on the friend's profile");
        }

        softAssertAll();
    }
}
