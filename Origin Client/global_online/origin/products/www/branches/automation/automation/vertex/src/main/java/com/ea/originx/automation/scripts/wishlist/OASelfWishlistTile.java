package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.InfoBubble;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
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
import java.util.Arrays;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests current user's 'Wishlist' tiles
 *
 * NEED UPDATE TO GDP
 *
 * @author RCHOI
 * @author sbentley
 */
public class OASelfWishlistTile extends EAXVxTestTemplate {

    @TestRail(caseId = 11704)
    @Test(groups = {"wishlist", "full_regression", "int_only"})
    public void testOASelfWishlistTile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final String userAccountName = userAccount.getUsername();

        EntitlementInfo vaultTitle = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String vaultTitleName = vaultTitle.getName();
        final String vaultTitleOfferID = vaultTitle.getOfferId();

        EntitlementInfo normalEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String normalEntitlementName = normalEntitlement.getName();
        final String normalEntitlementOfferID = normalEntitlement.getOfferId();

        EntitlementInfo virtualCurrency = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_17);
        final String virtualCurrencyOfferID = virtualCurrency.getOfferId();

        final EntitlementInfo thirdPartyeEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
        final String thirdPartyGameOfferID = thirdPartyeEntitlement.getOfferId();

        final List<String> entitlements = Arrays.asList(vaultTitleOfferID, virtualCurrencyOfferID, normalEntitlementOfferID, thirdPartyGameOfferID);

        logFlowPoint("Login to client using newly created account");  //1
        logFlowPoint("Add a vault, a normal, a third party and a virtual currency entitlement to 'Wishlist' and verify all 'Wishlist' items have correct packart"); //2
        logFlowPoint("Verify the all 'Wishlist' items have the correct product title"); //3
        logFlowPoint("Verify the all 'Wishlist' items have added date"); //4
        logFlowPoint("Verify the all 'Wishlist' items have 'Remove From 'Wishlist'' link"); //5
        logFlowPoint("Verify the all 'Wishlist' items have 'Buy' button"); //6
        logFlowPoint("Verify the all the items have price in their 'Buy' buttons"); //7
        logFlowPoint("Verify hovering on the 'Buy' button the game rating and 'EULA' link are visible"); //8
        logFlowPoint("Purchase 'Origin Access' for the user"); //9
        logFlowPoint("Navigate to current user's 'Wishlist' tab and Verify vault title has a 'Play access' button"); //10
        logFlowPoint("Verify vault title has a 'Buy' button as a second CTA"); //11
        logFlowPoint("Verify 'Added to Your Library' dialog appears when clicking 'Play with Access' button"); //12
        logFlowPoint("Close the dialog and verify PDP page shows up for entitlement after clicking the packart"); //13
        logFlowPoint("Navigate to 'Wishlist' tab and verified PDP shows up for entitlement after clicking the product title"); //14

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful by using new account: " + userAccountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + userAccountName);
        }

        //2
        // adding entitlements as a 'Wishlist' item
//        PDPHeroPackartRating pdpHeroPackartRating = new PDPHeroPackartRating(driver);
//        boolean thirdPartyAdded = MacroWishlist.addToWishlist(driver, thirdPartyeEntitlement);
//        boolean virtualCurrencyAdded = MacroWishlist.addToWishlist(driver, virtualCurrency);
//        String virtualCurrencyRating = pdpHeroPackartRating.getPDPHeroGameRating();
//        boolean normalEntitlementAdded = MacroWishlist.addToWishlist(driver, normalEntitlement);
//        String normalEntitlementRating = pdpHeroPackartRating.getPDPHeroGameRating();
//        boolean vaultTitleAdded = MacroWishlist.addToWishlist(driver, vaultTitle);
//        String vaultRating = pdpHeroPackartRating.getPDPHeroGameRating();
//
//        boolean addedToWishlist = thirdPartyAdded && virtualCurrencyAdded && normalEntitlementAdded && vaultTitleAdded;

        MacroWishlist.navigateToWishlist(driver);
        ProfileWishlistTab wishListTab = new ProfileWishlistTab(driver);
        boolean packArt = wishListTab.verifyPackArtsExist(vaultTitle, virtualCurrency, normalEntitlement, thirdPartyeEntitlement);
//        if (addedToWishlist && packArt) {
//            logPass("Added 'Wishlist' titles and verified the all 'Wishlist' items have the correct packart ");
//        } else {
//            logFailExit("Failed some 'Wishlist' items does not have the correct packart");
//        }

        //3
        boolean productTitle = wishListTab.verifyProductTitlesExist(vaultTitle, virtualCurrency, normalEntitlement, thirdPartyeEntitlement);
        if (productTitle) {
            logPass("Verified the all 'Wishlist' items have the correct product title");
        } else {
            logFail("Failed some 'Wishlist' items does not have the correct product title ");
        }

        //4
        boolean dateAdded = wishListTab.verifyDateAddedKeywordForAllItems();
        if (dateAdded) {
            logPass("Verified the all 'Wishlist' items have added date");
        } else {
            logFail("Failed some 'Wishlist' items does not have the correct added date ");
        }

        //5
        boolean removeFromWishListLink = wishListTab.verifyRemoveFromWishlistLinkExistForAllItems();
        if (removeFromWishListLink) {
            logPass("Verified the all 'Wishlist' items have 'Remove From 'Wishlist'' link");
        } else {
            logFailExit("Failed: some 'Wishlist' items does NOT have 'Remove From 'Wishlist'' link");
        }

        //6
        boolean buyButton = wishListTab.verifyBuyButtonsExistForAllItems();
        if (buyButton) {
            logPass("Verified the all 'Wishlist' items have 'Buy' button");
        } else {
            logFailExit("Failed: the some 'Wishlist' items do NOT have 'Buy' button");
        }

        //7
        boolean price = wishListTab.verifyPriceForBuyButtonsExist(entitlements);
        if (price) {
            logPass("Verified the all the items have price in their 'Buy' buttons");
        } else {
            logFailExit("Failed: The some 'Wishlist' items do not have price in their 'Buy' buttons");
        }

        //8
        // checking Rating and 'EULA' for tiles
        InfoBubble infoBubble = new InfoBubble(driver);
        WishlistTile vaultTile = wishListTab.getWishlistTile(vaultTitle);
        WishlistTile normalTile = wishListTab.getWishlistTile(normalEntitlement);
        WishlistTile VCTile = wishListTab.getWishlistTile(virtualCurrency);

//        vaultTile.hoverOnBuyButton();
//        boolean vaultRatingExist = infoBubble.getGameRating().equals(vaultRating);
//        boolean vaultTitleEULALinkExist = infoBubble.isEULAVisible();
//
//        VCTile.hoverOnBuyButton(); // Sometimes there is a timing issue with InfoBubble not updating when moving to another tile. Move to VC tile first then ba ck
//        normalTile.hoverOnBuyButton();
//        boolean normalEntitlementRatingExist = infoBubble.getGameRating().equals(normalEntitlementRating);
//        boolean normalEntitlementEULAExist = infoBubble.isEULAVisible();
//
//        VCTile.hoverOnBuyButton();
//        boolean virtualCurrencyRatingExist = infoBubble.getGameRating().equals(virtualCurrencyRating);
//        boolean virtualCurrencyeulaLinkExist = infoBubble.isEULAVisible();
//
//        boolean infoBubbleVisible = vaultRatingExist && vaultTitleEULALinkExist
//                && normalEntitlementRatingExist && normalEntitlementEULAExist
//                && virtualCurrencyRatingExist && virtualCurrencyeulaLinkExist;
//
//        if (infoBubbleVisible) {
//            logPass("Verified hovering on the 'Buy' button the game rating and 'EULA' link are visible");
//        } else {
//            logFailExit("Failed: hovering on the 'Buy' button the game rating and 'EULA' link are not visisble");
//        }

        //9
        if (MacroOriginAccess.purchaseOriginAccess(driver)) {
            logPass("Successfully purchased 'Origin Access' for the user  " + userAccountName);
        } else {
            logFail("Failed: Cannot purchase 'Origin Access' for the user " + userAccountName);
        }

        //10
        MacroWishlist.navigateToWishlist(driver);
        vaultTile = wishListTab.getWishlistTile(vaultTitleOfferID);
        normalTile = wishListTab.getWishlistTile(normalEntitlementOfferID);
        if (wishListTab.verifyPlayAccessButtonsExist(vaultTitle)) {
            logPass("Navigated to current user's 'Wishlist' tab and verified vault title: " + vaultTitleName + " has a 'Play access' button as main CTA");
        } else {
            logFailExit("Failed: vault title: " + vaultTitleName + " does NOT have a 'Play access' button as main CTA after Navigating to current user's 'Wishlist' tab");
        }

        //11
        if (wishListTab.verifyBuyButtonsExist(vaultTitle)) {
            logPass("Verified vault title: " + vaultTitleName + " has a 'Buy' button as a second CTA");
        } else {
            logFailExit("Failed: vault title: " + vaultTitleName + " does not have a 'Buy' button as a second CTA");
        }

        //12
        vaultTile.clickPlayAccessButton();
        CheckoutConfirmation addToLibraryDialog = new CheckoutConfirmation(driver);
        if (addToLibraryDialog.waitIsVisible()) {
            logPass("Verified 'added to your library' dialog appears when clicking 'click play with access' button");
        } else {
            logFailExit("Failed: 'added to your library' dialog does NOT appear when clicking 'click play with access' button");
        }

        //13
//       // PDPHeroDescription pdpHeroDescription = new PDPHeroDescription(driver);
//        addToLibraryDialog.close();
//        normalTile.clickPackArt();
//      //  pdpHeroDescription.waitForPdpHeroToLoad();
//        if ((Waits.pollingWait(() -> pdpHeroDescription.verifyGameTitle(normalEntitlementName)))) {
//            logPass("Closed the dialog and verified  PDP page shows up for entitlement: " + normalEntitlementName + " after clicking the packart");
//        } else {
//            logFailExit("Failed: unable to locate to PDP page for the entitlement: " + normalEntitlementName + "after closing dialog and clicking the partart");
//        }
//
//        //14
//        MacroWishlist.navigateToWishlist(driver);
//        normalTile = wishListTab.getWishlistTile(normalEntitlement);
//        normalTile.clickProductTitle();
//        pdpHeroDescription.waitForPdpHeroToLoad();
//        if ((Waits.pollingWait(() -> pdpHeroDescription.verifyGameTitle(normalEntitlementName)))) {
//            logPass("Navigated to 'Wishlist' tab and verified PDP page shows up for entitlement: " + normalEntitlementName + " after clicking the product title");
//        } else {
//            logFailExit("Failed: unable to locate to PDP page for the entitlement: " + normalEntitlementName + "after navigating to 'Wishlist' tab and clicking the product title");
//        }

        softAssertAll();
    }
}
