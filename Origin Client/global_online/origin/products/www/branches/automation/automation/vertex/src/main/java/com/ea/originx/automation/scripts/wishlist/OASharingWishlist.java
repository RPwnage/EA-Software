package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 *  A test which verifies that a user's 'Wishlist' is visible to everybody if
 *  their privacy settings are set to 'Everybody'
 *
 * @author cdeaconu
 */
public class OASharingWishlist extends EAXVxTestTemplate {
    
    @TestRail(caseId = 11703)
    @Test(groups = {"wishlist", "release_smoke"})
    public void testSharingWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccountA = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_ON_WISHLIST);
        final UserAccount userAccountB = AccountManager.getUnentitledUserAccount();
        final EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        
        // Clean friends
        userAccountA.cleanFriends();
        userAccountB.cleanFriends();

        logFlowPoint("Change the privacy of User A to 'Everyone' and log into Origin with User A."); // 1
        logFlowPoint("Navigate to User A's 'Wishlist' and get the URL for the 'Wishlist'."); // 2
        logFlowPoint("Logout of User A's account and log into Origin with User B."); // 3
        logFlowPoint("Search for User A using global search and go to their 'Wishlist' and verify the 'Wishlist' item is displayed."); // 4
        if(isClient) {
            logFlowPoint("Use the 'Share your Wishlist' URL in a browser and verify that user's are prompted to sign in to view wishlists"); // 5
        } else {
            logFlowPoint("Logout of User B's account use the 'Share your Wishlist' URL and verify that user's are prompted to sign in to view wishlists"); // 5
        }
        
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
        String wishlistShareAddressURL = MacroWishlist.getWishlistShareAddress(driver);
        boolean isWishlistUrlValid = EAXVxSiteTemplate.verifyUrlResponseCode(wishlistShareAddressURL);
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
        if (isClient) {
            settingsDriver.get(wishlistShareAddressURL);
            logPassFail(new ProfileHeader(settingsDriver).verifyOnSignInPage(), true);
        } else {
            new MiniProfile(driver).selectSignOut();
            driver.get(wishlistShareAddressURL);
            logPassFail(new ProfileHeader(driver).verifyOnSignInPage(), true);
        }
        
        softAssertAll();
    }
}