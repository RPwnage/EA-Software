package com.ea.originx.automation.scripts.feature.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import static com.ea.vx.originclient.templates.OATestBase.sleep;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests the wishlist privacy settings in regards to strangers and friends
 *
 * @author cvanichsarn
 */
public class OAWishlistPrivacy extends EAXVxTestTemplate {

    public void testWishlistPrivacy(ITestContext context, PrivacySetting setting) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_ON_WISHLIST);
        final UserAccount friend = AccountManager.getRandomAccount();
        final UserAccount friendOfFriend = AccountManager.getRandomAccount();
        final UserAccount stranger = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        user.cleanFriends();
        friend.cleanFriends();
        friendOfFriend.cleanFriends();
        stranger.cleanFriends();
        user.addFriend(friend);
        friend.addFriend(friendOfFriend);

        logFlowPoint("Login to Origin with user account A (account with an offer wishlisted)"); //1
        logFlowPoint("Open a browser and navigate to account A's privacy settings and set the profile visibility to " + setting.toString()); //2
        logFlowPoint("Navigate to account A's wishlist and verify an offer is wishlisted"); //3
        switch (setting) {
            case EVERYONE:
                logFlowPoint("Login on the web with the account D which is a stranger to account A"); //4a
                logFlowPoint("Verify that account D can view account A's wishlist using account A's wishlist share link"); //5a
                logFlowPoint("Sign out of account D and navigate to account A's wishlist link without signing into any accounts"); //6a
                break;
            case FRIENDS:
                logFlowPoint("Login on the web with account B which is a friend of account A"); //4b
                logFlowPoint("Verify that account B can view account A's wishlist using account A's wishlist share link"); //5b
                logFlowPoint("Sign out of account B and login to the web with account C which is a friend of a friend to account A"); //6b
                logFlowPoint("Verify that account C can't view account A's wishlist using account A's wishlist share link"); //7b
                break;
            case FRIENDS_OF_FRIENDS:
                logFlowPoint("Login on the web with account B which is a friend of account A"); //4c
                logFlowPoint("Verify that account B can view account A's wishlist using account A's wishlist share link"); //5c
                logFlowPoint("Sign out of account B and login to the web with account C which is a friend of a friend to account A"); //6c
                logFlowPoint("Verify that account C can view account A's wishlist using account A's wishlist share link"); //7c
                logFlowPoint("Sign out of account C and login on the web with the account D which is a stranger to account A"); //8c
                logFlowPoint("Verify that account D can't view account A's wishlist using account A's wishlist share link"); //9c
                break;
            case NO_ONE:
                logFlowPoint("Login on the web with account B which is a friend of account A"); //4d
                logFlowPoint("Verify that account B can't view account A's wishlist using account A's wishlist share link"); //5d
                break;
        }

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        final WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        logPassFail(MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, user, setting), true);
        client.getCompositeDriver().quitBrowserWebDriver();

        //3
        driver.navigate().refresh();
        Waits.pollingWait(() -> new DiscoverPage(driver).verifyDiscoverPageReached(), 10000, 1000, 0);
        MacroWishlist.navigateToWishlist(driver);
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        logPassFail(profileWishlistTab.verifyTilesExist(entitlement.getOfferId()), true);

        // This is where the test scenarios branch off
        final WebDriver driver2;
        final String wishlistAddr;
        switch (setting) {

            // Test case for OAWishlistPrivacyEveryone
            case EVERYONE:
                //4a
                wishlistAddr = MacroWishlist.getWishlistShareAddress(driver);
                driver2 = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
                driver2.get(driver.getCurrentUrl());
                logPassFail(MacroLogin.startLogin(driver2, stranger), true);
                //5a
                driver2.get(wishlistAddr);
                new ProfileHeader(driver2).waitForWishlistTabToLoad();
                logPassFail(new ProfileWishlistTab(driver2).verifyTilesExist(entitlement.getOfferId()), true);
                //6a
                new MiniProfile(driver2).selectSignOut();
                driver2.get(wishlistAddr);
                logPassFail(new ProfileHeader(driver2).verifyOnSignInPage(), true);
                break;

            // Test case for OAWishlistPrivacyFriends
            case FRIENDS:
                //4b
                wishlistAddr = MacroWishlist.getWishlistShareAddress(driver);
                driver2 = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
                driver2.get(driver.getCurrentUrl());
                logPassFail(MacroLogin.startLogin(driver2, friend), true);
                //5b
                driver2.get(wishlistAddr);
                new ProfileHeader(driver2).waitForWishlistTabToLoad();
                logPassFail(new ProfileWishlistTab(driver2).verifyTilesExist(entitlement.getOfferId()), true);
                //6b
                new MiniProfile(driver2).selectSignOut();
                driver2.get(OSInfo.getXURL()); //to bypass issues with hitting the 404 page after signout
                logPassFail(MacroLogin.startLogin(driver2, friendOfFriend), true);
                //7b
                driver2.get(wishlistAddr);
                new ProfileHeader(driver2).verifyOnProfilePage();
                sleep(1000);  //to be sure due to the waits on private profiles being rather inconsistent
                logPassFail(new ProfileHeader(driver2).verifyProfileIsPrivate(), true);
                break;

            // Test case for OAWishlistPrivacyFriendsOfFriends
            case FRIENDS_OF_FRIENDS:
                //4c
                wishlistAddr = MacroWishlist.getWishlistShareAddress(driver);
                driver2 = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
                driver2.get(driver.getCurrentUrl());
                logPassFail(MacroLogin.startLogin(driver2, friend), true);
                //5c
                driver2.get(wishlistAddr);
                new ProfileHeader(driver2).waitForWishlistTabToLoad();
                logPassFail(new ProfileWishlistTab(driver2).verifyTilesExist(entitlement.getOfferId()), true);
                //6c
                new MiniProfile(driver2).selectSignOut();
                driver2.get(OSInfo.getXURL()); //to bypass issues with hitting the 404 page after signout
                logPassFail(MacroLogin.startLogin(driver2, friendOfFriend), true);
                //7c
                driver2.get(wishlistAddr);
                new ProfileHeader(driver2).verifyOnProfilePage();
                logPassFail(new ProfileWishlistTab(driver2).verifyTilesExist(entitlement.getOfferId()), true);
                //8c
                new MiniProfile(driver2).selectSignOut();
                driver2.get(OSInfo.getXURL()); //to bypass issues with hitting the 404 page after signout
                logPassFail(MacroLogin.startLogin(driver2, stranger), true);
                //9c
                driver2.get(wishlistAddr);
                new ProfileHeader(driver2).verifyOnProfilePage();
                sleep(1000);  //to be sure due to the waits on private profiles being rather inconsistent
                logPassFail(new ProfileHeader(driver2).verifyProfileIsPrivate(), true);
                break;

            // Test case for OAWishlistPrivacyNoOne
            case NO_ONE:
                //4d
                driver2 = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
                driver2.get(driver.getCurrentUrl());
                logPassFail(MacroLogin.startLogin(driver2, friend), true);
                //5d
                new MiniProfile(driver2).selectViewMyProfile();
                ProfileHeader profileHeader2 = new ProfileHeader(driver2);
                profileHeader2.openFriendsTab();
                ProfileFriendsTab profileFriendsTab = new ProfileFriendsTab(driver2);
                profileHeader2.verifyFriendsTabActive();
                profileFriendsTab.clickFriendTile(user.getUsername());
                profileHeader2.verifyOnProfilePage();
                profileHeader2.openWishlistTabPrivate();
                logPassFail(profileHeader2.verifyProfileIsPrivate(), true);
                break;
        }
        
        softAssertAll();
    }
}
