package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for adding and removing wishlist items from the profile wishlist tab
 *
 * @author palui
 */
public class OAWishlistAddRemoveTile extends EAXVxTestTemplate {

    @TestRail(caseId = 11700)
    @Test(groups = {"wishlist", "full_regression"})
    public void testWishlistAddRemoveTile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement1 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementName1 = entitlement1.getName();
        final String entitlementOfferId1 = entitlement1.getOfferId();
        EntitlementInfo entitlement2 = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
        final String entitlementName2 = entitlement2.getName();
        final String entitlementOfferId2 = entitlement2.getOfferId();

        UserAccount user = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String username = user.getUsername();

        logFlowPoint("Launch Origin and login as newly registered user: " + username); //1
        logFlowPoint(String.format("Open PDP page for '%1$s'. From the 'Buy' button dropdown, add '%1$s' to the wishlist", entitlementName1)); //2
        logFlowPoint(String.format("Open PDP page for '%1$s'. From the 'Buy' button dropdown, add '%1$s' to the wishlist", entitlementName2)); //3
        logFlowPoint(String.format("Open profile for '%s' and navigate to the 'Wishlist' tab", username)); //4
        logFlowPoint(String.format("Verify '%s' and '%s' tiles appear in the 'Wishlist' tab", entitlementName1, entitlementName2)); //5
        logFlowPoint(String.format("Click 'Remove from wishlist' at '%1$s' tile. Verify '%1$s' has been removed from (and '%2$s' remains in) the user's profile 'Wishlist' tab",
                entitlementName1, entitlementName2)); //6
        logFlowPoint(String.format("Exit Origin and Log back in as user: %s", username)); //7
        logFlowPoint(String.format("Open profile for '%s' and navigate to the 'Wishlist' tab", username)); //8
        logFlowPoint(String.format("Verify only '%s' remains under the 'Wishlist' tab", entitlementName2)); //9
        logFlowPoint(String.format("Open PDP page for '%1$s'. From the 'Buy' button dropdown, add '%1$s' to the wishlist", entitlementName1)); //10
        logFlowPoint(String.format("Open profile for '%s' and navigate to the 'Wishlist' tab", username)); //11
        logFlowPoint(String.format("Verify '%s' and '%s' tiles appear in the 'Wishlist' tab", entitlementName1, entitlementName2)); //12

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Verified login successful as newly registered user: " + username);
        } else {
            logFailExit("Failed: Cannot login as newly registered user: " + username);
        }

        //2
        if (MacroWishlist.addToWishlist(driver, entitlement1)) {
            logPass(String.format("Verified '%s' has been added (through the PDP dropdown) to the wishlist'", entitlementName1));
        } else {
            logFailExit(String.format("Failed: Cannot add '%s' (through the PDP dropdown) to the wishlist", entitlementName1));
        }

        //3
        if (MacroWishlist.addToWishlist(driver, entitlement2)) {
            logPass(String.format("Verified '%s' has been added (through the PDP dropdown) to the wishlist", entitlementName2));
        } else {
            logFailExit(String.format("Failed: Cannot add '%s' (through the PDP dropdown) to the wishlist", entitlementName2));
        }

        //4
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        if (profileHeader.verifyWishlistActive()) {
            logPass(String.format("Verified 'Wishlist' tab opens at the 'Profile' page of %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot open the 'Wishlist' tab at the 'Profile' page of %s", username));
        }

        //5
        profileHeader.waitForWishlistTabToLoad();
        ProfileWishlistTab wishlistTab = new ProfileWishlistTab(driver);
        boolean isAdded = Waits.pollingWait(() -> wishlistTab.verifyTilesExist(entitlementOfferId1, entitlementOfferId2));
        if (isAdded) {
            logPass(String.format("Verified '%s' and '%s' appear in the 'Wishlist' tab", entitlementName1, entitlementName2));
        } else {
            logFailExit(String.format("Failed: '%s' and/or '%s' do not appear in the 'Wishlist' tab", entitlementName1, entitlementName2));
        }

        //6
        WishlistTile wishlistTile1 = wishlistTab.getWishlistTile(entitlement1);
        wishlistTile1.clickRemoveFromWishlistLink();
        boolean isRemoved1 = Waits.pollingWait(() -> !wishlistTab.verifyTilesExist(entitlementOfferId1));
        boolean isPresent2 = Waits.pollingWait(() -> wishlistTab.verifyTilesExist(entitlementOfferId2));
        if (isRemoved1 && isPresent2) {
            logPass(String.format("Verified '%s' has been removed from (and '%s' remains in) the user's profile 'Wishlist' tab",
                    entitlementName1, entitlementName2));
        } else {
            logFailExit(String.format("Failed: Cannot remove '%s' from the user's profile 'Wishlist' tab, or '%s' does not remains in the tab",
                    entitlementName1, entitlementName2));
        }

        //7
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        if (isClient) {
            new MainMenu(driver).selectExit();
            client.stop();
            driver = startClientObject(context, client);
        } else {
            new MiniProfile(driver).selectSignOut();
        }
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Verified exit and re-login successful as " + username);
        } else {
            logFailExit("Failed: Cannot exit and re-login as " + username);
        }

        //8
        new MiniProfile(driver).selectViewMyProfile();
        profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        if (profileHeader.verifyWishlistActive()) {
            logPass(String.format("Verified 'Wishlist' tab opens at the 'Profile' page of %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot open the 'Wishlist' tab at the 'Profile' page of %s", username));
        }

        //9
        ProfileWishlistTab wishlistTab2 = new ProfileWishlistTab(driver);
        isRemoved1 = Waits.pollingWait(() -> !wishlistTab2.verifyTilesExist(entitlementOfferId1));
        isPresent2 = Waits.pollingWait(() -> wishlistTab2.verifyTilesExist(entitlementOfferId2));
        if (isRemoved1 && isPresent2) {
            logPass(String.format("Verified only '%s' remains in the user's profile 'Wishlist' tab",
                    entitlementName2));
        } else {
            logFailExit(String.format("Failed: Cannot remove '%s' from the user's profile 'Wishlist' tab, or '%s' does not remains in the tab",
                    entitlementName1, entitlementName2));
        }

        //10
        // There is an apprent bug preventing PDP from luanching with protocol call on some machines. Search PDP by name instead
        if (MacroWishlist.addToWishlist(driver, entitlement1)) {
            logPass(String.format("Verified '%s' has been added (through the PDP dropdown) to the wishlist", entitlementName1));
        } else {
            logFailExit(String.format("Failed: Cannot add '%s' (through the PDP dropdown) to the wishlist", entitlementName1));
        }

        //11
        new MiniProfile(driver).selectViewMyProfile();
        profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        if (profileHeader.verifyWishlistActive()) {
            logPass(String.format("Verified 'Wishlist' tab opens at the 'Profile' page of %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot open the 'Wishlist' tab at the 'Profile' page of %s", username));
        }

        //12
        profileHeader.waitForWishlistTabToLoad();
        ProfileWishlistTab wishlistTab3 = new ProfileWishlistTab(driver);
        boolean isAdded1 = Waits.pollingWait(() -> wishlistTab3.verifyTilesExist(entitlementOfferId1));
        isPresent2 = Waits.pollingWait(() -> wishlistTab3.verifyTilesExist(entitlementOfferId2));
        if (isAdded1 && isPresent2) {
            logPass(String.format("Verified '%s' and '%s' appear in the 'Wishlist' tab", entitlementName1, entitlementName2));
        } else {
            logFailExit(String.format("Failed: '%s' and/or '%s' do not appear in the 'Wishlist' tab", entitlementName1, entitlementName2));
        }

        softAssertAll();
    }
}
