package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;

import java.util.Arrays;
import java.util.List;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

import static com.ea.originx.automation.lib.resources.games.EntitlementId.QA_OFFER_2_UNRELEASED_STANDARD;

/**
 * Tests the other's user wishlist for an ineligible wishlist items and an empty
 * wishlist
 *
 * @author nvarthakavi
 */
public class OAOtherUserWishlistIneligible extends EAXVxTestTemplate {

    @TestRail(caseId = 159743)
    @Test(groups = {"wishlist", "full_regression", "int_only"})
    public void testOtherUserWishlistIneligible(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final UserAccount otherUserAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount usaAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_USA);
        final UserAccount noWishlistUserAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount senderAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final String otherUserAccountName = otherUserAccount.getUsername();
        final String senderAccountName = senderAccount.getUsername();
        final String usaAccountName = usaAccount.getUsername();
        final String noWishlistUserAccountName = noWishlistUserAccount.getUsername();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String entitlementName = entitlement.getName();
        final EntitlementInfo coinsPackEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_17_DELUXE);
        final String coinsPackEntitlementName = coinsPackEntitlement.getName();
        final EntitlementInfo thirdPartyEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
        final String thirdPartyEntitlementName = thirdPartyEntitlement.getName();
        final EntitlementInfo preorderEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(QA_OFFER_2_UNRELEASED_STANDARD);
        final List<String> entitlements = Arrays.asList(entitlement.getOfferId(), coinsPackEntitlement.getOfferId(), thirdPartyEntitlement.getOfferId(), preorderEntitlement.getOfferId());

        logFlowPoint("Launch Origin and login as usernoWishlistAccount and Navigate to the Wishlist page to copy the share your link"
                + "(Add Item->Copy Link->Remove Item)"); //1
        logFlowPoint("Logout from noWishlistAccount and login to '%s'usaAccount"); //2
        logFlowPoint(String.format("Add '%s' wishlist from their corresponding PDP Page and Navigate to usaAccount Wishlist"
                + "to verify the entitlements are added and to copy the wishlist link ", entitlementName)); //3
        logFlowPoint("Logout from usaAccount and login to ineligibiltyCheckAccount"); //4
        logFlowPoint(String.format("Purchase origin access to the user '%s'", otherUserAccountName)); // 5
        logFlowPoint("Add a regular game,coins pack, a pre-order game and third-party game to wishlist "
                + "and navigate to user wishlist to verify the entitlements are added a and to copy the share your wishlist link"); // 6
        logFlowPoint("Logout from ineligibiltyCheckAccount and login to senderAccount "); //7
        logFlowPoint(String.format("Purchase '%s' entitlement as a gift for ineligibiltyCheckAccount "
                + "to verify a unopened gift is ineligibile to send as a gift again to the same user  ", entitlementName)); //8
        if (isClient) {
            logFlowPoint("Navigate to ineligibiltyCheckAccount Wishlist Page"); // 9
        } else {
            logFlowPoint("Navigate to ineligibiltyCheckAccount Wishlist Page and open copied share your wishlist link in the current window"); // 9
        }
        logFlowPoint("Verify all the entitlements (unopened gift,coins pack, third party entitlement and pre-order entitlement) are ineligible for ineligibiltyCheckAccount "); //10
        if (isClient) {
            logFlowPoint("Navigate to usaAccount Wishlist Page"); // 11
        } else {
            logFlowPoint("Navigate to usaAccount Wishlist Page and open copied share your wishlist link in the current window"); // 11
        }
        logFlowPoint(String.format("Verify the giftable entitlement '%s' is ineligible to gift for a user '%s' from another country ", entitlementName, usaAccountName)); //12
        if (isClient) {
            logFlowPoint("Navigate to noWishlistAccount Wishlist Page"); // 13
        } else {
            logFlowPoint("Navigate to noWishlistAccountWishlist Page and open copied share your wishlist link in the current window"); // 13
        }
        logFlowPoint("Verify that if the other user's wishlist noWishlistAccount is empty, it will display a message indicating there is nothing to display "); //14

        //1
        final WebDriver driver = startClientObject(context, client);
        boolean isLoginSuccessful = MacroLogin.startLogin(driver, noWishlistUserAccount);
        MacroWishlist.addToWishlist(driver, entitlement); //The share your link is not visible until there is an item in the wihslist
        // (So adding and remving the item for a empty wishlist user)
        new MiniProfile(driver).selectViewMyProfile();
        new ProfileHeader(driver).openWishlistTab();
        String wishlistAddrNoWishlist = MacroWishlist.getWishlistShareAddress(driver);
        if (isLoginSuccessful && wishlistAddrNoWishlist != null) {
            logPass("Verified login successful to user account: " + noWishlistUserAccountName + "and navigated successfull to copy the share your link");
        } else {
            logFailExit("Failed: Cannot login to user account: " + noWishlistUserAccountName + "and could not navigat successfull to copy the share your link");
        }

        //2
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, usaAccount)) {
            logPass("Verified login successful to user account: " + usaAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + usaAccountName);
        }

        //3
        MacroWishlist.addToWishlist(driver, entitlement);
        new MiniProfile(driver).selectViewMyProfile();
        new ProfileHeader(driver).openWishlistTab();
        ProfileWishlistTab profileWishlistTab1 = new ProfileWishlistTab(driver);
        String wishlistAddrUSA = MacroWishlist.getWishlistShareAddress(driver);
        boolean entitlementExists = profileWishlistTab1.verifyTilesExist(entitlement.getOfferId());
        if (wishlistAddrUSA != null && entitlementExists) {
            logPass("Successfully opened the Wishlist Tab and copied the share your link for the user "
                    + usaAccountName + " and verified " + entitlementName + " is added to the wishlist");
        } else {
            logFailExit("Failed To open the Wishlist Tab and to copy the share your link for the user "
                    + usaAccountName + " and could not verify " + entitlementName + " is added to the wishlist");
        }

        //4
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, otherUserAccount)) {
            logPass("Verified login successful to user account: " + otherUserAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + otherUserAccountName);
        }

        // clean and add both accounts as friends
        senderAccount.cleanFriends();
        UserAccountHelper.addFriends(senderAccount, otherUserAccount, noWishlistUserAccount, usaAccount);

        //5
        if (MacroOriginAccess.purchaseOriginAccess(driver)) {
            logPass("Successfully purchased origin access for the user  " + otherUserAccountName);
        } else {
            logFailExit("Failed: Cannot purchase origin access for the user " + otherUserAccountName);
        }

        //6
        MacroWishlist.addToWishlist(driver, entitlement);
        MacroWishlist.addToWishlist(driver, thirdPartyEntitlement);
        MacroWishlist.addToWishlist(driver, coinsPackEntitlement);
        MacroWishlist.addToWishlist(driver, preorderEntitlement);
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.waitForAchievementsTabToLoad();
        profileHeader.openWishlistTab();
        ProfileWishlistTab profileWishlistTab2 = new ProfileWishlistTab(driver);
        String wishlistAddrOtherUser = MacroWishlist.getWishlistShareAddress(driver);
        boolean isWishlistItems = profileWishlistTab2.verifyTilesExist(entitlement.getOfferId(), thirdPartyEntitlement.getOfferId(), coinsPackEntitlement.getOfferId(), preorderEntitlement.getOfferId());
        if (wishlistAddrOtherUser != null && isWishlistItems) {
            logPass("Successfully opened the Wishlist Tab and copied the share your link for the user "
                    + otherUserAccountName + " and verified all the entitlements are added to the wishlist");
        } else {
            logFailExit("Failed To open the Wishlist Tab and to copy the share your link for the user "
                    + otherUserAccountName + " and could not verify all the entitlements are added to the wishlist");
        }

        //7
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, senderAccount)) {
            logPass("Verified login successful to user account: " + senderAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + senderAccountName);
        }

        //8
        if (MacroGifting.purchaseGiftForFriend(driver, entitlement, otherUserAccountName, "This is a gift")) {
            logPass("Verified successful purchase a gift for to user account: " + otherUserAccountName);
        } else {
            logFailExit("Failed: To purchase a gift for to user account: " + otherUserAccountName);
        }

        //9
        ProfileHeader wishlistProfile = new ProfileHeader(driver);
        if (isClient) {
            MacroProfile.navigateToFriendProfile(driver, otherUserAccountName);
            wishlistProfile.openWishlistTab();
        } else {
            driver.get(wishlistAddrOtherUser);
            wishlistProfile.waitForWishlistTabToLoad();
        }
        if (wishlistProfile.verifyWishlistActive()) {
            logPass("Successfully opened the Wishlist Tab for the user " + otherUserAccountName);
        } else {
            logFailExit("Failed To open the Wishlist Tab for the user " + otherUserAccountName);
        }

        //10
        ProfileWishlistTab otherUserWishlistTab = new ProfileWishlistTab(driver);
        boolean ineligibleOtherUser = otherUserWishlistTab.verifyIneligibleMessageExist(entitlements);
        if (ineligibleOtherUser) {
            logPass("Verified the all wishlist items are ineligible ");
        } else {
            logFailExit("Failed some wishlist are not ineligible ");
        }

        //11
        ProfileHeader wishlistProfile1 = new ProfileHeader(driver);
        if (isClient) {
            MacroProfile.navigateToFriendProfile(driver, usaAccountName);
            wishlistProfile1.openWishlistTab();
        } else {
            driver.get(wishlistAddrUSA);
            wishlistProfile1.waitForWishlistTabToLoad();
        }
        if (wishlistProfile1.verifyWishlistActive()) {
            logPass("Successfully opened the Wishlist Tab for the user " + usaAccountName);
        } else {
            logFailExit("Failed To open the Wishlist Tab for the user " + usaAccountName);
        }

        //12
        ProfileWishlistTab usaWishlistTab = new ProfileWishlistTab(driver);
        boolean ineligibleUSA = usaWishlistTab.verifyIneligibleMessageExist(entitlement);
        if (ineligibleUSA) {
            logPass("Verified that the giftable item for an user from other country wishlist is ineligible ");
        } else {
            logFailExit("Failed the gifitable wishlist item for an user from other country is not ineligible ");
        }

        //13
        ProfileHeader wishlistProfile2 = new ProfileHeader(driver);
        if (isClient) {
            MacroProfile.navigateToFriendProfile(driver, noWishlistUserAccountName);
            wishlistProfile2.openWishlistTab();
        } else {
            driver.get(wishlistAddrNoWishlist);
            wishlistProfile2.waitForWishlistTabToLoad();
        }
        if (wishlistProfile2.verifyWishlistActive()) {
            logPass("Successfully opened the Wishlist Tab for the user " + noWishlistUserAccountName);
        } else {
            logFailExit("Failed To open the Wishlist Tab for the user " + noWishlistUserAccountName);
        }

        //14
        boolean isEmpty = new ProfileWishlistTab(driver).isEmptyWishlist(driver);
        if (isEmpty) {
            logPass("Verified that if the other user's wishlist is empty, a message indicating there is nothing to display is visible ");
        } else {
            logFailExit("Failed if the other user's wishlist is empty, a message indicating there is nothing to display is  not visible  ");
        }

        softAssertAll();
    }
}
