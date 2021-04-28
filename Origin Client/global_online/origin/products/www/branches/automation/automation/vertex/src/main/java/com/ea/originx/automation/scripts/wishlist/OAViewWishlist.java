package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Viewing items on the Wishlist
 *
 * @author tdhillon
 */
public class OAViewWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 1016699)
    @Test(groups = {"wishlist", "release_smoke", "int_only"})
    public void testViewWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();

        final String discountedEntitlementName = Sims4.SIMS4_DISCOUNT_TEST_DLC_NAME;
        final String discountedEntitlementOfferID = Sims4.SIMS4_DISCOUNT_TEST_DLC_OFFER_ID;
        final String discountedEntitlementPartialURL = Sims4.SIMS4_DISCOUNT_TEST_DLC_PARTIAL_PDP_URL;

        final EntitlementInfo vaultEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        final String vaultEntitlementName = vaultEntitlement.getName();
        final String vaultEntitlementParentName = vaultEntitlement.getParentName();
        final String vaultEntitlementOfferID = vaultEntitlement.getOfferId();

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.MINI_METRO);
        final String entitlementOfferID = entitlement.getOfferId();
        final String entitlementName = entitlement.getName();

        logFlowPoint("Create an account and log into Origin");  // 1
        logFlowPoint("Add three entitlements to the wishlist"); // 2
        logFlowPoint("Navigate to your wishlist and verify all wishlist items have the visible packart"); // 3
        logFlowPoint("Verify all wishlist items have the correct product title"); // 4
        logFlowPoint("Verify clicking packart bring you to the PDP for " + vaultEntitlementName); // 5
        logFlowPoint("Verify that the date the game was added on the wishlist is displayed"); // 6
        logFlowPoint("Navigate back to the wishlist and verify clicking on the title brings you to the PDP for " + discountedEntitlementName); // 7
        logFlowPoint("Verify discount is visible for " + discountedEntitlementName + " in PDP page"); // 8
        logFlowPoint("Navigate back to wishlist tile again and purchase the game " + entitlementName + " through buy CTA"); // 9
        logFlowPoint("Purchase Origin Access"); // 10
        logFlowPoint("Verify that if the user is a subscriber viewing a vault game, they are displayed with a split CTA"); // 11

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        boolean addDiscountEntitlement = MacroWishlist.addToWishlist(driver, discountedEntitlementName, discountedEntitlementOfferID, discountedEntitlementPartialURL);
        boolean addVaultEntitlement = MacroWishlist.addToWishlist(driver, vaultEntitlement);
        boolean addEntitlement = MacroWishlist.addToWishlist(driver, entitlement);
        logPassFail(addDiscountEntitlement && addVaultEntitlement && addEntitlement, true);

        // 3
        MacroWishlist.navigateToWishlist(driver);
        ProfileWishlistTab wishListTab = new ProfileWishlistTab(driver);
        logPassFail(wishListTab.verifyPackArtExistsEntitlement(discountedEntitlementOfferID, discountedEntitlementName)
                && wishListTab.verifyPackArtsExist(vaultEntitlement, entitlement), true);

        // 4
        boolean isDiscountedTitleCorrect = wishListTab.verifyProductTitleEntitlement(discountedEntitlementOfferID, discountedEntitlementName);
        boolean isEntitlementOfferIDTitleCorrect = wishListTab.verifyProductTitleEntitlement(entitlementOfferID, entitlementName);
        boolean isVaultTitleCorrect = wishListTab.verifyProductTitleEntitlement(vaultEntitlementOfferID, vaultEntitlementName);
        logPassFail(isDiscountedTitleCorrect && isEntitlementOfferIDTitleCorrect && isVaultTitleCorrect, true);

        // 5
        WishlistTile wishlistVaultTile = wishListTab.getWishlistTile(vaultEntitlementOfferID);
        wishlistVaultTile.clickPackArt();
        logPassFail(new GDPHeader(driver).verifyGameTitle(vaultEntitlementParentName), true);

        // 6
        MacroWishlist.navigateToWishlist(driver);
        logPassFail(wishListTab.verifyDateAddedKeywordForAllItems(), true);

        // 7
        wishListTab.getWishlistTile(discountedEntitlementOfferID).clickProductTitle();
        logPassFail(new GDPHeader(driver).verifyGameTitle(discountedEntitlementName), true);


        // 8
        logPassFail(new GDPHeroActionDescriptors(driver).verifyOriginalPriceIsStrikedThrough(), true);

        // 9
        MacroWishlist.navigateToWishlist(driver);
        WishlistTile wishlistTile = wishListTab.getWishlistTile(entitlement.getOfferId());
        wishlistTile.clickBuyButton();
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        // 10
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        // 11
        MacroWishlist.navigateToWishlist(driver);
        boolean isPlayWithAccessButtonVisible = wishlistVaultTile.verifyPlayWithAccessButtonExists();
        boolean isBuyButtonVisible = wishlistVaultTile.verifyBuyButtonVisible();
        logPassFail(isPlayWithAccessButtonVisible && isBuyButtonVisible, true);

        softAssertAll();
    }
}