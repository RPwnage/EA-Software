package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests removing items from a user's wishlist
 *
 * @author caleung
 */
public class OARemoveWishlistItems extends EAXVxTestTemplate {

    @TestRail(caseId = 1016700)
    @Test(groups = {"release_smoke", "wishlist", "int_only"})
    public void testRemoveWishlistItems(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final OriginClient friendClient = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final String environment = OSInfo.getEnvironment();
        UserAccount userAccountA;
        UserAccount userAccountB;
         if (environment.equalsIgnoreCase("production")) {
            userAccountA = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());
            userAccountB = AccountManagerHelper.createNewThrowAwayAccount(CountryInfo.CountryEnum.CANADA.getCountry());
        } else {
            userAccountA = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(CountryInfo.CountryEnum.CANADA.getCountry());
            userAccountB = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.NO_ENTITLEMENTS, "Canada");
            userAccountB.cleanFriends();
            userAccountB.addFriend(userAccountA);
        }
        final EntitlementInfo firstEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final EntitlementInfo secondEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS3_STARTER_PACK);
        final EntitlementInfo thirdEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);

        logFlowPoint("Create an account (User A) and login to Origin."); // 1
        logFlowPoint("Add three entitlements to User A's wishlist."); // 2
        logFlowPoint("Log into Origin with User B."); // 3
        logFlowPoint("Send one of the entitlements on User A's wishlist to User A as a gift."); // 4
        logFlowPoint("From User A navigate to wishlist."); // 5
        logFlowPoint("Remove one of the entitlements from User A's wishlist and verify it is removed."); // 6
        logFlowPoint("Purchase one of the entitlements from User A's wishlist and verify it is removed."); // 7
        logFlowPoint("Click on buy CTA for gifted entitlement and verify it is removed through gifting flow."); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccountA), true);

        // 2
        final boolean addFirstEntitlement = MacroWishlist.addToWishlist(driver, firstEntitlement);
        final boolean addSecondEntitlement = MacroWishlist.addToWishlist(driver, secondEntitlement);
        final boolean addThirdEntitlement = MacroWishlist.addToWishlist(driver, thirdEntitlement);
        logPassFail(addFirstEntitlement && addSecondEntitlement && addThirdEntitlement, true);

        // 3
        WebDriver friendDriver = startClientObject(context, friendClient);
        logPassFail(MacroLogin.startLogin(friendDriver, userAccountB), true);
        if (environment.equalsIgnoreCase("production")) {
            MacroSocial.addFriendThroughUI(friendDriver, driver, userAccountB, userAccountA);
        }
        
        // 4
        MacroWishlist.navigateToFriendWishlist(friendDriver, userAccountA.getUsername());
        boolean purchaseSuccessful;
        if (environment.equalsIgnoreCase("production")) {
            purchaseSuccessful = MacroWishlist.purchaseGiftFromWishListTab(friendDriver, thirdEntitlement, userAccountB.getEmail(), "Sims 4", false, true);
        } else {
            purchaseSuccessful = MacroWishlist.purchaseGiftFromWishListTab(friendDriver, thirdEntitlement, userAccountA.getUsername(), "Sims 4", false, false);
        }
        logPassFail(purchaseSuccessful, true);

        // 5
        logPassFail(MacroWishlist.navigateToWishlist(driver), true);

        // 6
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        WishlistTile secondEntitlementTile = profileWishlistTab.getWishlistTile(secondEntitlement);
        String secondEntitlementOfferId = secondEntitlementTile.getOfferId();
        secondEntitlementTile.clickRemoveFromWishlistLink();
        boolean tileExists = profileWishlistTab.verifyTilesExist(secondEntitlementOfferId);
        logPassFail(!tileExists, false);

        // 7
        WishlistTile firstEntitlementTile = profileWishlistTab.getWishlistTile(firstEntitlement);
        String firstEntitlementOfferId = firstEntitlementTile.getOfferId();
        firstEntitlementTile.clickBuyButton();
        if (environment.equalsIgnoreCase("production")) {
            MacroPurchase.purchaseThroughPaypalOffCodeFromPaymentInformationPage(driver, firstEntitlement, userAccountA.getEmail());
        } else {
            MacroPurchase.completePurchaseAndCloseThankYouPage(driver);
        }
        Waits.sleep(5000); // wait because it takes a while for entitlement to get removed
        tileExists = profileWishlistTab.verifyTilesExist(firstEntitlementOfferId);
        logPassFail(!tileExists, false);

        // 8
        WishlistTile thirdEntitlementTile = profileWishlistTab.getWishlistTile(thirdEntitlement);
        thirdEntitlementTile.clickBuyButton();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        openGiftPage.waitForVisible();
        openGiftPage.clickCloseButton();
        if (isClient) {
            new MainMenu(driver).selectRefresh(); // refresh because entitlement not getting removed after accepting gift
        } else {
            driver.get(driver.getCurrentUrl()); // refresh on browser
        }
        MacroWishlist.navigateToWishlist(driver);
        tileExists = profileWishlistTab.verifyTilesExist(thirdEntitlement.getOfferId());
        logPassFail(!tileExists, false);

        softAssertAll();
    }
}