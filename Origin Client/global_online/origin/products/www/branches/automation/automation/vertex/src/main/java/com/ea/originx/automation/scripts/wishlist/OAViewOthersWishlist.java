package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.webdrivers.BrowserType;
import com.google.common.base.Strings;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test which verifies that a user's 'Wishlist' is displayed or not displayed
 * according to their privacy settings
 *
 * @author caleung
 */
public class OAViewOthersWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 1016701)
    @Test(groups = {"wishlist", "release_smoke"})
    public void testViewOthersWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccountA = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_ON_WISHLIST);
        final UserAccount userAccountB = AccountManager.getUnentitledUserAccount();
        EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        // Setup friends
        userAccountA.cleanFriends();
        userAccountB.cleanFriends();
        userAccountA.addFriend(userAccountB);

        logFlowPoint("Change the privacy of User A to 'Everyone' and log into Origin with User A."); // 1
        logFlowPoint("Navigate to User A's 'Wishlist' and get the URL for the 'Wishlist'."); // 2
        logFlowPoint("Logout of User A's account and log into Origin with User B."); // 3
        logFlowPoint("Search for User A using global search and go to their 'Wishlist' and verify the 'Wishlist' item is displayed."); // 4
        logFlowPoint("Change the privacy of User A to 'No one'."); // 5
        logFlowPoint("Navigate to User A's 'Wishlist' and verify the 'Wishlist' items are not shown."); // 6

        // 1
        WebDriver driver = startClientObject(context, client);
        WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        boolean isPrivacySettingChangeSuccessful = MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, userAccountA, PrivacySettingsPage.PrivacySetting.EVERYONE);
        boolean isLoginSuccessful = MacroLogin.startLogin(driver, userAccountA);
        logPassFail(isPrivacySettingChangeSuccessful && isLoginSuccessful, true);

        // 2
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        boolean isWishlistActive = profileHeader.verifyWishlistActive();
        boolean isWishlistUrlValid = !Strings.isNullOrEmpty(MacroWishlist.getWishlistShareAddress(driver));
        logPassFail(isWishlistActive && isWishlistUrlValid, true);

        // 3
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, userAccountB), true);

        // 4
        MacroSocial.navigateToUserProfileThruSearch(driver, userAccountA);
        ProfileHeader profileHeaderAccountA = new ProfileHeader(driver);
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        profileHeaderAccountA.openWishlistTab();
        logPassFail(profileHeaderAccountA.verifyWishlistActive() && profileWishlistTab.verifyTilesExist(entitlementInfo.getOfferId()), true);

        // 5
        logPassFail(MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, userAccountA, PrivacySettingsPage.PrivacySetting.NO_ONE), true);
        settingsDriver.close();
        // adding this step because privacy settings refresh after login
        new MiniProfile(driver).selectSignOut();
        MacroLogin.startLogin(driver, userAccountB);

        // 6
        MacroSocial.navigateToUserProfileThruSearch(driver, userAccountA);
        profileHeaderAccountA.openWishlistTabPrivate();
        logPassFail(profileHeaderAccountA.verifyWishlistActive() && profileHeaderAccountA.verifyProfileIsPrivate(), true);

        softAssertAll();
    }
}
