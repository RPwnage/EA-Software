package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the profile games tab when the user has games.
 *
 * @author lscholte
 */
public class OAProfileWithGames extends EAXVxTestTemplate {

    @TestRail(caseId = 13069)
    @Test(groups = {"full_regression", "social"})
    public void testProfileWithGames(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final int numberOfEntitlements = 3;
        final AccountTags tag = AccountTags.THREE_ENTITLEMENTS;
        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(tag);
        UserAccount friendAccount = AccountManagerHelper.getTaggedUserAccount(tag);

        userAccount.cleanFriends();
        friendAccount.cleanFriends();

        userAccount.addFriend(friendAccount);

        logFlowPoint("Login to Origin as " + userAccount.getUsername()); //1
        logFlowPoint("Open the profile using the mini profile menu"); //2
        logFlowPoint("Navigate to the 'Games' tab on the profile"); //3
        logFlowPoint("Verify the 'No Games' message is not visible"); //4
        logFlowPoint("Verify that three entitlements are displayed in the 'Games' tab"); //5
        logFlowPoint("Verify that the entitlements are DiP Small, Non-DiP Small, and DiP Large"); //6
        logFlowPoint("Verify there is a 'Learn More' link on each entitlement"); //7

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
        if (!gamesTab.verifyNoGamesMessage(true)) {
            logPass("The 'No Games' message does not appear");
        } else {
            logFail("The 'No Games' message appears");
        }

        //5
        if (gamesTab.verifyMyGamesHeaderNumber(numberOfEntitlements, numberOfEntitlements)) {
            logPass("The 'My Games' header correctly displays " + numberOfEntitlements + " entitlements");
        } else {
            logFail("The 'My Games' header does not display " + numberOfEntitlements + " entitlements");
        }

        //6
        boolean dipSmallExists = gamesTab.verifyEntitlementExists(EntitlementInfoConstants.DIP_SMALL_NAME);
        boolean nonDipSmallExists = gamesTab.verifyEntitlementExists(EntitlementInfoConstants.NON_DIP_SMALL_NAME);
        boolean dipLargeExists = gamesTab.verifyEntitlementExists(EntitlementInfoConstants.DIP_LARGE_NAME);
        if (dipSmallExists && nonDipSmallExists && dipLargeExists) {
            logPass("All entitlements are displayed on the profile");
        } else {
            logFail("Not all entitlements are displayed on the profile");
        }

        //7
        if (gamesTab.verifyAllEntitlementsHaveLearnMore(numberOfEntitlements)) {
            logPass("There is a 'Learn More' link on all entitlements");
        } else {
            logFail("There is not a 'Learn More' link on all entitlements");
        }
        softAssertAll();
    }
}
