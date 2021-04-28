package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for removing wishlist items by purchasing it
 *
 * @author palui
 */
public class OAWishlistRemoveByPurchase extends EAXVxTestTemplate {

    @TestRail(caseId = 11701)
    @Test(groups = {"wishlist", "full_regression", "int_only"})
    public void testWishlistRemoveByPurchase(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.createNewThrowAwayAccount("Canada");
        final String username = user.getUsername();

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_ICON);
        final String entitlementName = entitlement.getName();

        EntitlementInfo wishEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String wishName = wishEntitlement.getName();
        final String wishOfferId = wishEntitlement.getOfferId();

        EntitlementInfo digitalCurrency = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_CURRENCY_PACK);
        final String digitalCurrencyName = digitalCurrency.getName();
        final String digitalCurrencyOfferId = digitalCurrency.getOfferId();
        final String digitalCurrencyOcdPath = digitalCurrency.getOcdPath();
        final String digitalCurrencyPartialPDPUrl = digitalCurrency.getPartialPdpUrl();

        logFlowPoint("Launch Origin and login as newly registered user"); //1
        logFlowPoint("Purchase game 1 that has digital currency extra content"); //2
        logFlowPoint("Add game 2 and a digital currency entitlement of game 1 to the wishlist"); //3
        logFlowPoint("Verify both game 2 and digital currency entitlement appear in the Profile 'Wishlist' tab"); //4
        logFlowPoint("Purchase game 2 and verify it is removed from the user's wishlist"); //5
        logFlowPoint("Verify purchased game 2 cannot be added back to the wishlist"); //6
        logFlowPoint("Purchase digital currency entitlement and verify it is NOT removed from the user's wishlist"); //7

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Verified login successful to newly registered user: " + username);
        } else {
            logFailExit("Failed: Cannot login to newly registered user: " + username);
        }

        //2
        if (MacroPurchase.purchaseEntitlement(driver, entitlement)) {
            logPass(String.format("Verified game 1 '%s' has been purchased successfully", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot purchase game 1 '%s'", entitlementName));
        }

        //3
        boolean game2Added = MacroWishlist.addToWishlist(driver, wishEntitlement);
        boolean dcAdded = MacroWishlist.addToWishlist(driver, digitalCurrency);
        if (game2Added && dcAdded) {
            logPass(String.format("Verified game 2 '%s' and digital currency entitlement '%s' have been added to the wishlist", wishName, digitalCurrencyName));
        } else {
            logFailExit(String.format("Failed: Cannot add game 2 '%s' or digital currency entitlement '%s' to the wishlist", wishName, digitalCurrencyName));
        }

        //4
        MacroWishlist.navigateToWishlist(driver);
        ProfileWishlistTab wishlistTab = new ProfileWishlistTab(driver);
        boolean bothAppearInWishlistTab = Waits.pollingWait(() -> wishlistTab.verifyTilesExist(wishOfferId, digitalCurrencyOfferId));
        if (bothAppearInWishlistTab) {
            logPass(String.format("Verified game 2 '%s' and digital currency entitlement '%s' appear in user's Profile 'Wishlist' tab", wishName, digitalCurrencyName));
        } else {
            logFailExit(String.format("Failed: Either or both '%s' and '%s' do not appear in user's Profile 'Wishlist' tab", wishName, digitalCurrencyName));
        }

        //5
        boolean game2Purchased = MacroPurchase.purchaseEntitlement(driver, wishEntitlement);
        MacroWishlist.navigateToWishlist(driver);
        boolean game2Removed = Waits.pollingWait(() -> !wishlistTab.verifyTilesExist(wishOfferId));
        boolean dcRemains = Waits.pollingWait(() -> wishlistTab.verifyTilesExist(digitalCurrencyOfferId));
        if (game2Purchased && game2Removed & dcRemains) {
            logPass(String.format("Verified purchasing game 2 '%s' removes it from (with digital currency entitlement '%s' remains in) the wishlist",
                    wishName, digitalCurrencyName));
        } else {
            String purchaseResult = (game2Purchased ? "Can" : "Cannot") + " purchase game 2 '%s', or ";
            String baseResult = (game2Removed ? "can" : "cannot") + " remove game 2 from the wishlist, or ";
            String dcResult = "digital currency entitlement '%s' " + (dcRemains ? "remains" : "does not remain") + " in the wishlist";
            logFailExit(String.format("Failed: " + purchaseResult + baseResult + dcResult, wishName, digitalCurrencyName));
        }

        //6
        boolean game2CanBeAdded = MacroWishlist.verifyAddingGameToWishlist(driver, wishEntitlement);
        if (!game2CanBeAdded) {
            logPass(String.format("Verified purchased game 2 '%s' cannot be added back to the wishlist", wishName));
        } else {
            logFail(String.format("Failed: Purchased game 2 '%s' can still be added back to the wishlist", wishName));
        }

        //7
        boolean dcPurchased = MacroPurchase.purchaseEntitlement(driver, digitalCurrencyName, digitalCurrencyOfferId, digitalCurrencyOcdPath, digitalCurrencyPartialPDPUrl);
        MacroWishlist.navigateToWishlist(driver);
        boolean dcNotRemoved = Waits.pollingWait(() -> wishlistTab.verifyTilesExist(digitalCurrencyOfferId));
        if (dcPurchased && dcNotRemoved) {
            logPass(String.format("Verified purchasing digital currency entitlement '%s' does NOT remove it from the user's profile 'Wishlist' tab", digitalCurrencyName));
        } else {
            String purchaseResult = (dcPurchased ? "Can" : "Cannot") + " purchase digital currency entitlement '%s', or purchasing it ";
            String dcResult = (dcNotRemoved ? "does not" : "does") + " remove it from the wishlist";
            logFailExit(String.format("Failed: " + purchaseResult + dcResult, digitalCurrencyName));
        }
    }
}
