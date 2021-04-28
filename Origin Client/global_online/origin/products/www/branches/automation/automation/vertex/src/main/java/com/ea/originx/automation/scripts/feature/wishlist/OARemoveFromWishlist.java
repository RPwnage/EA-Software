package com.ea.originx.automation.scripts.feature.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests removing items from the wishlist tab
 *
 * @author tdhillon
 */
public class OARemoveFromWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 3120667)
    @Test(groups = {"wishlist_smoke", "wishlist", "allfeaturesmoke", "int_only"})
    public void testRemoveFromWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userA = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount userB = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final String userAName = userA.getUsername();

        //Entitlements
        final EntitlementInfo removeFromWishlistEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18);
        final String removeFromWishlistEntitlementOfferId = removeFromWishlistEntitlement.getOfferId();
        final EntitlementInfo purchaseFromWishlistEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BIG_MONEY);
        final String purchaseFromWishlistEntitlementOfferId = purchaseFromWishlistEntitlement.getOfferId();
        final EntitlementInfo purchaseFromPDPEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String purchaseFromPDPEntitlementOfferId = purchaseFromPDPEntitlement.getOfferId();
        final EntitlementInfo purchaseAsGiftEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String purchaseAsGiftEntitlementOfferId = purchaseAsGiftEntitlement.getOfferId();

        logFlowPoint("Launch Origin and login with user A"); // 1
        logFlowPoint("Add four entitlements to user A's wishlist from their PDP pages and navigate to user A's wishlist");  // 2
        logFlowPoint("Verify the entitlements are added to the wishlist"); // 3
        logFlowPoint("Remove the first entitlement from the wishlist by clicking the 'remove from wishlist' link and verify it is successfully removed"); // 4
        logFlowPoint("Purchase the second entitlement by clicking the 'Buy' button in the wishlist tab and verify it is successfully removed from the wishlist"); // 5
        logFlowPoint("Navigate to the PDP page of the third entitlement, purchase the entitlement, and verify it is successfully removed from the wishlist"); // 6
        logFlowPoint("Logout from user A's account and login to user B's account"); // 7
        logFlowPoint("Navigate to the PDP page of the fourth entitlement and buy it as a gift for user A"); // 8
        logFlowPoint("Logout from user B's account, login to user A's account, and open the gift"); // 9
        logFlowPoint("Navigate to the wishlist tab and verify that the fourth entitlement has been removed"); // 10

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userA), true);

        // clean and add both accounts as friends
        userA.cleanFriends();
        userB.cleanFriends();
        userA.addFriend(userB);

        //2
        boolean isFirstEntitlementAddedToWishlist = MacroWishlist.addToWishlist(driver, removeFromWishlistEntitlement);
        boolean isSecondEntitlementAddedToWishlist = MacroWishlist.addToWishlist(driver, purchaseFromWishlistEntitlement);
        boolean isThirdEntitlementAddedToWishlist =  MacroWishlist.addToWishlist(driver, purchaseFromPDPEntitlement);
        boolean isFourthEntitlementAddedToWishlist = MacroWishlist.addToWishlist(driver, purchaseAsGiftEntitlement);
        boolean isEntitlementsAddedToWishlistSuccessful = isFirstEntitlementAddedToWishlist && isSecondEntitlementAddedToWishlist
                && isThirdEntitlementAddedToWishlist && isFourthEntitlementAddedToWishlist;
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        boolean isWishlistActive = profileHeader.verifyWishlistActive();
        logPassFail(isEntitlementsAddedToWishlistSuccessful && isWishlistActive, true);

        //3
        profileHeader.waitForWishlistTabToLoad();
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        EAXVxSiteTemplate.refreshAndSleep(driver);
        // Wait added to account for slower loading of the Wishlist page
        boolean verifyWishlistEntitlementsSuccessfulAdded = Waits.pollingWait(() -> profileWishlistTab.verifyTilesExist(removeFromWishlistEntitlementOfferId,
                purchaseFromWishlistEntitlementOfferId, purchaseFromPDPEntitlementOfferId, purchaseAsGiftEntitlementOfferId), 30000, 3000, 0);
        logPassFail(verifyWishlistEntitlementsSuccessfulAdded, true);

        //4
        WishlistTile removeFromWishlistEntitlementTile = profileWishlistTab.getWishlistTile(removeFromWishlistEntitlement);
        removeFromWishlistEntitlementTile.clickRemoveFromWishlistLink();
        logPassFail(!profileWishlistTab.verifyTilesExist(removeFromWishlistEntitlementOfferId), true);

        //5
        WishlistTile bigMoneyTile = profileWishlistTab.getWishlistTile(purchaseFromWishlistEntitlement);
        bigMoneyTile.clickBuyButton();
        MacroPurchase.completePurchaseAndCloseThankYouPage(driver);
        EAXVxSiteTemplate.refreshAndSleep(driver);
        logPassFail(!profileWishlistTab.verifyTilesExist(purchaseFromWishlistEntitlementOfferId), true);

        //6
        MacroPurchase.purchaseEntitlement(driver, purchaseFromPDPEntitlement);
        miniProfile.waitForProfileToLoad(); // Wait for page in Chrome to adjust after purchasing the entitlement
        miniProfile.selectViewMyProfile();
        profileHeader.openWishlistTab();
        logPassFail(!profileWishlistTab.verifyTilesExist(purchaseFromPDPEntitlementOfferId), true);

        //7
        miniProfile.selectSignOut();
        Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL);
        logPassFail(MacroLogin.startLogin(driver, userB), true);

        //8
        logPassFail(MacroGifting.purchaseGiftForFriend(driver, purchaseAsGiftEntitlement, userAName, "This is a gift"), true);

        //9
        miniProfile.waitForProfileToLoad(); // Wait for page in Chrome to adjust after purchasing the gift for a friend
        miniProfile.selectSignOut();
        Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL);
        MacroLogin.startLogin(driver, userA);
        MacroCommon.openGiftAddToLibrary(driver);
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(new GameLibrary(driver).isGameTileVisibleByOfferID(purchaseAsGiftEntitlementOfferId), true);

        //10
        miniProfile.selectViewMyProfile();
        profileHeader.openWishlistTab();
        logPassFail(!profileWishlistTab.verifyTilesExist(purchaseAsGiftEntitlementOfferId), true);

        softAssertAll();
    }
}