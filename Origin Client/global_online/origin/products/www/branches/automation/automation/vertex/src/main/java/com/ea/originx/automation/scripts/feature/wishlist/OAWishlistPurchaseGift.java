package com.ea.originx.automation.scripts.feature.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests adding and purchasing entitlement as wishlist from current user and
 * friend
 *
 * @author rocky
 */
public class OAWishlistPurchaseGift extends EAXVxTestTemplate {

    @TestRail(caseId = 3121260)
    @Test(groups = {"wishlist", "wishlist_smoke", "int_only", "client_only", "allfeaturesmoke"})
    public void testWishlistPurchaseGift(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount wishlistReceiver = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String wishlistReceiverName = wishlistReceiver.getUsername();
        final UserAccount wishlistSender = AccountManagerHelper.getUnentitledUserAccountWithCountry("Canada");
        final String wishlistSenderName = wishlistSender.getUsername();

        final EntitlementInfo entitlement1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BIG_MONEY);
        final String entitlement1Name = entitlement1.getName();
        final EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlement2Name = entitlement2.getName();

        logFlowPoint("Login to client using newly created account: " + wishlistReceiverName + " as wishlist receiver"); // 1
        logFlowPoint("Verify two wishlist :" + entitlement1Name + " and " + entitlement2Name + " are successfully added"); // 2
        logFlowPoint("Verify two wishlist: " + entitlement1Name + " and " + entitlement2Name + " in wishlist tab in profile page"); // 3
        logFlowPoint("Login to client using wishlistSender: " + wishlistSenderName); // 4
        logFlowPoint("Purchase the wishlist " + entitlement2Name + " for friend and sign out"); // 5
        logFlowPoint("Login to client using account: " + wishlistReceiverName + " as wishlist receiver"); // 6
        logFlowPoint("Verify " + entitlement1Name + "and " + entitlement2Name + " exist in the game library"); // 7
        logFlowPoint("Download and install " + entitlement1Name); // 8
        logFlowPoint("Verify user is able to launch " + entitlement1Name); // 9

        //1
        final WebDriver driver = startClientObject(context, client);
        boolean wishlistReceiverRegistered = MacroLogin.startLogin(driver, wishlistReceiver);
        wishlistSender.cleanFriends();
        wishlistSender.addFriend(wishlistReceiver);
        if (wishlistReceiverRegistered) {
            logPass("Logged into client using newly created account: " + wishlistReceiverName + " as wishlist receiver");
        } else {
            logFailExit("Failed to login to client using newly created account: " + wishlistReceiverName + " as wishlist receiver");
        }

        //2
        boolean addWishList1 = MacroWishlist.addToWishlist(driver, entitlement1);
        boolean addWishList2 = MacroWishlist.addToWishlist(driver, entitlement2);
        if (addWishList1 && addWishList2) {
            logPass("Verified two wishlist :" + entitlement1Name + " and " + entitlement2Name + " are successfully added");
        } else {
            logFailExit("Failed to add two wishlist :" + entitlement1Name + " or " + entitlement2Name);
        }

        //3
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.waitForAchievementsTabToLoad();
        profileHeader.openWishlistTab();
        profileHeader.waitForWishlistTabToLoad();
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        boolean foundEntitlement1 = profileWishlistTab.verifyTilesExist(entitlement1.getOfferId());
        boolean foundEntitlement2 = profileWishlistTab.verifyTilesExist(entitlement2.getOfferId());
        if (foundEntitlement1 && foundEntitlement2) {
            logPass("Verified two wishlist :" + entitlement1Name + " and " + entitlement2Name + " in wishlist tab in profile page");
        } else {
            logFailExit("Could not find two wishlist :" + entitlement1Name + " or " + entitlement2Name + " in wishlist tab in profile page");
        }

        //4
        profileWishlistTab.getWishlistTile(entitlement1).clickBuyButton();
        MacroPurchase.completePurchaseAndCloseThankYouPage(driver);
        NavigationSidebar navBar = new NavigationSidebar(driver);
        navBar.gotoDiscoverPage();
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, wishlistSender)) {
            logPass("Logged into client successfully using user account: " + wishlistSenderName + " as wishlist sender");
        } else {
            logFailExit("Failed: Cannot login to user account: " + wishlistSenderName + " as wishlist sender");
        }

        //5
        MacroProfile.navigateToFriendProfile(driver, wishlistReceiverName);
        profileHeader.waitForAchievementsTabToLoad();
        ProfileHeader wishlistProfile = new ProfileHeader(driver);
        wishlistProfile.openWishlistTab();
        profileHeader.waitForWishlistTabToLoad();
        if (MacroWishlist.purchaseGiftFromWishListTab(driver, entitlement2, wishlistReceiverName, "test", false, false)) {
            logPass("Successfully purchased the wishlist " + entitlement2Name + " for friend");
        } else {
            logFailExit("Could not purchase the wishlist " + entitlement2Name + " for friend");
        }
        navBar.gotoDiscoverPage();
        new MiniProfile(driver).selectSignOut();

        //6
        if (MacroLogin.startLogin(driver, wishlistReceiver)) {
            logPass("Verified login successful to user account: " + wishlistReceiverName + " as wishlist receiver");
        } else {
            logFailExit("Failed: Cannot login to user account: " + wishlistReceiverName + " as wishlist receiver");
        }

        //7
        DiscoverPage discoverPage = navBar.gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        discoverPage.waitForPageToLoad();
        MacroCommon.openGiftAddToLibrary(driver);
        GameLibrary gameLibrary = new GameLibrary(driver);
        boolean isEnt1Exist = gameLibrary.isGameTileVisibleByOfferID(entitlement1.getOfferId());
        boolean isEnt2Exist = gameLibrary.isGameTileVisibleByOfferID(entitlement2.getOfferId());
        if (isEnt1Exist && isEnt2Exist) {
            logPass("Verified " + entitlement1Name + "and " + entitlement2Name + "+ exist in the game library");
        } else {
            logFailExit("Failed to find " + isEnt1Exist + " or " + isEnt2Exist + "+ in the game library");
        }

        //8
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlement1.getOfferId())) {
            logPass("Download and install " + entitlement1Name);
        } else {
            logFailExit("Failed to download/install " + entitlement1Name);
        }

        //9
        GameTile gameTile = new GameTile(driver, entitlement1.getOfferId());
        gameTile.play();
        boolean entitlementRunning = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, entitlement1.getProcessName()));
        if (entitlementRunning) {
            logPass("Verified user is able to launch " + entitlement1Name);
        } else {
            logFailExit("Failed to launch " + entitlement1Name);
        }

        softAssertAll();

    }
}
