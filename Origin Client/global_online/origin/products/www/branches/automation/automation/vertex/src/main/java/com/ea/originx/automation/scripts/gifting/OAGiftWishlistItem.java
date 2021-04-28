package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.OriginClientConstants;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the unveiling opens for a user when he tries to buy the entitlement
 * from the wishlist
 *
 * @author nvarthakavi
 */
public class OAGiftWishlistItem extends EAXVxTestTemplate {

    @TestRail(caseId = 11794)
    @Test(groups = {"wishlist", "gifting", "full_regression", "int_only"})
    public void testGiftWishlistItem(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount wishlistAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount senderAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA);
        final String wishlistAccountName = wishlistAccount.getUsername();
        final String senderAccountName = senderAccount.getUsername();
        EntitlementInfo battlefieldEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String battlefieldEntitlementName = battlefieldEntitlement.getName();

        logFlowPoint(String.format("Launch Origin and login as user '%s'", wishlistAccountName)); //1
        logFlowPoint(String.format("Add '%s' to wishlist from their corresponding PDP Page and Navigate to '%s' wishlist page in MyProfile"
                + " and verify the '%s' is added to the wishlist", battlefieldEntitlementName, wishlistAccountName, battlefieldEntitlementName)); //2
        logFlowPoint(String.format("Logout from '%s' and login to '%s'", wishlistAccountName, senderAccountName)); //3
        logFlowPoint(String.format("Navigate to the PDP page of '%s' and buy as gift for '%s'", battlefieldEntitlementName, wishlistAccountName)); //4
        logFlowPoint(String.format("Logout from '%s' and login to '%s' ", senderAccountName, wishlistAccountName)); //5
        logFlowPoint(String.format("Navigate to '%s' wishlist page in MyProfile and click on buy button of '%s' and verify the unveiling opens", wishlistAccountName, battlefieldEntitlementName)); //6
        logFlowPoint(String.format("Verify '%s' is added to the gamelibrary ", battlefieldEntitlementName)); //7

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, wishlistAccount), true);

        // clean and add both accounts as friends
        senderAccount.cleanFriends();
        senderAccount.addFriend(wishlistAccount);

        //2
        MacroWishlist.addToWishlist(driver, battlefieldEntitlement);
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        profileHeader.openWishlistTab();
        profileHeader.waitForWishlistTabToLoad();
        logPassFail(profileHeader.verifyWishlistActive() && profileWishlistTab.verifyTilesExist(battlefieldEntitlement.getOfferId()), true);

        //3
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, senderAccount), true);

        //4
        logPassFail(MacroGifting.purchaseGiftForFriend(driver, battlefieldEntitlement, wishlistAccountName, "This is a gift"), true);

        //5
        new MiniProfile(driver).selectSignOut();
        logPassFail(MacroLogin.startLogin(driver, wishlistAccount), true);

        //6
        MacroWishlist.navigateToWishlist(driver);
        profileWishlistTab.getWishlistTile(battlefieldEntitlement).clickBuyButton();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        logPassFail(openGiftPage.waitForVisible(), true);

        //7
        openGiftPage.clickCloseButton();
        // wait for 'Open Gift Page' to close
        sleep(2000);
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(battlefieldEntitlement.getOfferId()), true);

        softAssertAll();
    }
}
