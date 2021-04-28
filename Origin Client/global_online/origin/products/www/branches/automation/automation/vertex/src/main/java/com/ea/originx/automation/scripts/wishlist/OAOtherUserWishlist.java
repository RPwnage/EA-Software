package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.InfoBubble;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.util.ArrayList;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the other user's wishlist(privacy setting as everyone)
 *
 * @author nvarthakavi
 * @author sbentley
 */
public class OAOtherUserWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 11719)
    @Test(groups = {"wishlist", "full_regression"})
    public void testOtherUserWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount wishlistAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount friendAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final String wishlistAccountName = wishlistAccount.getUsername();
        final String friendAccountName = friendAccount.getUsername();
        UserAccountHelper.addFriends(wishlistAccount, friendAccount);

        final EntitlementInfo firstEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final EntitlementInfo secondEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final EntitlementInfo thirdEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final EntitlementInfo fourthEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        final List<String> entitlements = new ArrayList<>();
        entitlements.add(firstEntitlement.getOfferId());
        entitlements.add(secondEntitlement.getOfferId());
        entitlements.add(fourthEntitlement.getOfferId());

        logFlowPoint("Launch Origin and login");    // 1
        logFlowPoint("Navigate to PDP and purchase entitlement");   // 2
        logFlowPoint("Add products to 'Wishlist'");   // 3
        logFlowPoint("Navigate to 'Wishlist' and verify all products are added"); // 4
        logFlowPoint("Logout and login to friend's account");   // 5
        logFlowPoint("Purchase Origin Access"); // 6
        logFlowPoint("Navigate to first account's 'Wishlist' Page and verify the packart is displayed for each entitlements");  // 7
        logFlowPoint("Verify product title, including edition, is displayed for each item");    // 8
        logFlowPoint("Verify that the added on <date> is displayed for each item"); // 9
        logFlowPoint("Verify that a 'Purchase as Gift' CTA button is displayed for each item on the other user's wishlist and Verify that the a la carte and in vault state don't appear");   // 10
        logFlowPoint("Verify that the Price is displayed in the CTA button for each item"); // 11
        logFlowPoint("Verify 'Remove from 'Wishlist'' button is not present on each tile"); // 12
        logFlowPoint("Verify that upon hover of the buy button will display the 'Game Raiting' and 'EULA' link");    // 13
        logFlowPoint("Verify subscriber discounts are displayed for applicable items and the discount is applied on the CTA");  // 14

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, wishlistAccount)) {
            logPass("Verified login successful to user account: " + wishlistAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + wishlistAccountName);
        }

        // 2
        if (MacroPurchase.purchaseEntitlement(driver, thirdEntitlement)) {
            logPass("Successfully purchased the entitlement" + thirdEntitlement.getName());
        } else {
            logFailExit("Failed To purchase the entitlement" + thirdEntitlement.getName());
        }

        // 3
//        //PDPHeroPackartRating pdpHeroPackartRating = new PDPHeroPackartRating(driver);
//        final boolean addedFirstEntitlement = MacroWishlist.addToWishlist(driver, secondEntitlement);
//        String secondEntitlementRating = pdpHeroPackartRating.getPDPHeroGameRating();
//        final boolean addedSecondEntitlement = MacroWishlist.addToWishlist(driver, firstEntitlement);
//        String firstEntitlementRating = pdpHeroPackartRating.getPDPHeroGameRating();
//        final boolean addedThirdEntitlement = MacroWishlist.addToWishlist(driver, fourthEntitlement);
//        String fourthEntitlementRating = pdpHeroPackartRating.getPDPHeroGameRating();
//        if (addedFirstEntitlement && addedSecondEntitlement && addedThirdEntitlement) {
//            logPass(String.format("Successfully added products to 'Wishlist': &s, &s, &s", firstEntitlement.getName(), secondEntitlement.getName(), fourthEntitlement.getName()));
//        } else {
//            logFailExit(String.format("Failed to add products to 'Wishlist': &s, &s, &s", firstEntitlement.getName(), secondEntitlement.getName(), fourthEntitlement.getName()));
//        }

        // 4
        MacroWishlist.navigateToWishlist(driver);
        ProfileHeader profileHeader1 = new ProfileHeader(driver);
        profileHeader1.waitForWishlistTabToLoad();
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        if (profileWishlistTab.verifyProductTitlesExist(firstEntitlement, secondEntitlement, fourthEntitlement)) {
            logPass("Successfully verified products on 'Wishlist'");
        } else {
            logFailExit("Failed to verify products on 'Wishlist'");
        }

        // 5
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, friendAccount)) {
            logPass("Verified login successful to user account: " + friendAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + friendAccountName);
        }

        // 6
        if (MacroOriginAccess.purchaseOriginAccess(driver)) {
            logPass("Successfully purchased origin access for the user  " + friendAccountName);
        } else {
            logFailExit("Failed: Cannot purchase origin access for the user " + friendAccountName);
        }

        // 7
        MacroWishlist.navigateToFriendWishlist(driver, wishlistAccountName);
        ProfileWishlistTab otherUserWishlistTab = new ProfileWishlistTab(driver);
        if (otherUserWishlistTab.verifyPackArtsExist(firstEntitlement, secondEntitlement, fourthEntitlement)) {
            logPass("Verified the all wishlist items have the correct packart ");
        } else {
            logFail("Failed some wishlist items does not have the correct packart");
        }

        // 8
        if (otherUserWishlistTab.verifyProductTitlesExist(firstEntitlement, secondEntitlement, fourthEntitlement)) {
            logPass("Verified the all wishlist items have the correct product title ");
        } else {
            logFail("Failed some wishlist items does not have the correct product title ");
        }

        // 9
        boolean dateAdded = otherUserWishlistTab.verifyDateAddedKeywordForAllItems();
        if (dateAdded) {
            logPass("Verified the all wishlist items have added date");
        } else {
            logFail("Failed some wishlist items does not have the correct added date ");
        }

        // 10
        boolean buyAsGiftButtonsExist = otherUserWishlistTab.verifyPurchaseAsAGiftButtonsExist(entitlements);
        boolean buyButtonsExists = otherUserWishlistTab.verifyBuyButtonsExist(entitlements);
        boolean playAccessButtonsExists = otherUserWishlistTab.verifyPlayAccessButtonsExist(entitlements);
        boolean buyButtonsOK = buyAsGiftButtonsExist
                && !buyButtonsExists
                && !playAccessButtonsExists;
        if (buyButtonsOK) {
            logPass("Verified the all the items have only purchase as a gift buttons");
        } else {
            logFail("Failed: The some wishlist items have purchase as a gift button or buy button ");
        }

        // 11
        boolean price = otherUserWishlistTab.verifyPriceForPurchaseAsGiftButtonsExist(entitlements);
        if (price) {
            logPass("Verified the all the items have price in their buy buttons");
        } else {
            logFail("Failed: The some wishlist items do not have price in their buy buttons");
        }

        // 12
        if (!profileWishlistTab.verifyRemoveFromWishlistLinkExistForAllItems()) {
            logPass("Successfully verified 'Remove From 'Wishlist'' does not exist");
        } else {
            logFail("Failed to verify 'Remove From 'Wishlist'' does not exist");
        }

        // 13
        InfoBubble infoBubble = new InfoBubble(driver);
        WishlistTile firstEntitlementTile = otherUserWishlistTab.getWishlistTile(firstEntitlement.getOfferId());
        WishlistTile secondEntitlementTile = otherUserWishlistTab.getWishlistTile(secondEntitlement.getOfferId());
        WishlistTile fourthEntitlementTile = otherUserWishlistTab.getWishlistTile(fourthEntitlement.getOfferId());

//        firstEntitlementTile.hoverOnPurchaseAsGiftButton();
//        boolean verifyFirstEntitlementRating = infoBubble.getGameRating().equals(firstEntitlementRating);
//        boolean verifyFirstEntitlementEULA = infoBubble.isEULAVisible();
//        secondEntitlementTile.hoverOnPurchaseAsGiftButton();
//        boolean verifySecondEntitlementRating = infoBubble.getGameRating().equals(secondEntitlementRating);
//        boolean verifySecondEntitlementEULA = infoBubble.isEULAVisible();
//        fourthEntitlementTile.hoverOnPurchaseAsGiftButton();
//        boolean verifyFourthEntitlementRating = infoBubble.getGameRating().equals(fourthEntitlementRating);
//        boolean verifyFourthEntitlementEULA = infoBubble.isEULAVisible();
//
//        boolean infoBubbleVisible
//                = verifyFirstEntitlementRating
//                && verifyFirstEntitlementEULA
//                && verifySecondEntitlementRating
//                && verifySecondEntitlementEULA
//                && verifyFourthEntitlementRating
//                && verifyFourthEntitlementEULA;
//
//        if (infoBubbleVisible) {
//            logPass("Verified on hovering on the purchase as a gift button the 'Game Raiting' and 'EULA' link are visisble");
//        } else {
//            logFail("Failed: On hovering on the purchase as a gift button the 'Game Raiting' and 'EULA' link are not visisble");
//        }

        // 14
        boolean originAccessDiscount
                = firstEntitlementTile.verifyOriginAccessDiscountIsVisible()
                && secondEntitlementTile.verifyOriginAccessDiscountIsVisible()
                && fourthEntitlementTile.verifyOriginAccessDiscountIsVisible();

        if (originAccessDiscount) {
            logPass("Verified Origin Access discount seen on 'Wishlist'");
        } else {
            logFail("Failed to find Origin Access discount on 'Wishlist'");
        }

        softAssertAll();
    }
}
