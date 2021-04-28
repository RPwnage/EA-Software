package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test PDP Dropdown for giftable and non-giftable games, as well as PDP
 * wishlist message
 *
 * NEEDS UPDATE TO GDP
 *
 * @author palui
 */
public class OAWishlistPDPDropdown extends EAXVxTestTemplate {

    @TestRail(caseId = 40226)
    @Test(groups = {"gifting", "wishlist", "full_regression", "int_only"})
    public void testWishlistPDPDropdown(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo giftableGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String giftableGameName = giftableGame.getName();
        EntitlementInfo nonGiftableGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
        final String nonGiftableGameName = nonGiftableGame.getName();

        UserAccount user = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final String username = user.getUsername();

        logFlowPoint("Launch Origin and login as newly registered user: " + username); //1
        logFlowPoint(String.format("Open PDP page for giftable game '%s'. Verify 'Buy' button dropdown has 'Purchase As Gift' option", giftableGameName)); //2
        logFlowPoint("Click 'Purchase As Gift' option. Verify the 'Friends Selection' dialog opens"); //3
        logFlowPoint(String.format("Verify PDP page for '%s' has 'Buy' button dropdown with 'Add To Wishlist' option", giftableGameName)); //4
        logFlowPoint(String.format("Click 'Add to wishlist' option. "
                + "Verify that the wishlist heart icon, 'On your wishlist' message for '%s', and 'View full wishlist' link are visible", giftableGameName)); //5
        logFlowPoint(String.format("Open PDP page for non-giftable game '%s'. "
                + "Verify 'Buy' button dropdown does not have 'Purchase As Gift' option", nonGiftableGameName)); //6

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Verified login successful as newly registered user: " + username);
        } else {
            logFailExit("Failed: Cannot login as newly registered user: " + username);
        }

        //2
       // boolean pdpPageLoaded = MacroPDP.loadPdpPage(driver, giftableGame);
      //  PDPHeroActionCTA pdpHeroCTA = new PDPHeroActionCTA(driver);
        // pdpHeroCTA.waitForPdpHeroToLoad();
    //    boolean dropdownAppears = pdpHeroCTA.verifyBuyDropdownArrowVisible();
        //     boolean giftingOptionAvailable = pdpHeroCTA.verifyBuyDropdownPurchaseAsGiftItemAvailable();
//        if (pdpPageLoaded && dropdownAppears && giftingOptionAvailable) {
//            logPass(String.format("Verified PDP page for giftable '%s' has 'Buy' button dropdown with 'Purchase As Gift' option", giftableGameName));
//        } else {
//            logFailExit(String.format("Failed: PDP page for giftable '%s' does not have 'Buy' button dropdown or does not have 'Purchase As Gift' option", giftableGameName));
//        }

        //3
    //    pdpHeroCTA.selectBuyDropdownPurchaseAsGift();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass("Verified selecting 'Purchase As Gift' option opens the 'Friends Selection' dialog");
        } else {
            logFailExit("Failed: Selecting 'Purchase As Gift' option does not open the 'Friends Selection' dialog");
        }
        friendsSelectionDialog.closeAndWait();

        //4
//        boolean wishlistingOptionAvailable = pdpHeroCTA.verifyBuyDropdownAddToWishlistItemAvailable();
//        if (wishlistingOptionAvailable) {
//            logPass(String.format("Verified PDP page for giftable '%s' has 'Buy' button dropdown with 'Add to Wishlist' option", giftableGameName));
//        } else {
//            logFailExit(String.format("Failed: PDP page for giftable '%s' does not have 'Buy' button dropdown with 'Add to Wishlist' option", giftableGameName));
//        }

        //5
        MacroWishlist.addToWishlist(driver, giftableGame);
//        if (new PDPHeroActionDescriptors(driver).verifyOnWishlist(giftableGameName)) {
//            logPass(String.format("Verified clicking 'Add to wishlist' option displays the wishlist heart icon, 'On your wishlist' message for '%s', and 'View full wishlist' link", giftableGameName));
//        } else {
//            logFailExit(String.format("Failed: Clicking 'Add to wishlist' option does not display the wishlist heart icon, 'On your wishlist' message for '%s', or 'View full wishlist' link", giftableGameName));
//        }

        //6
//        pdpPageLoaded = MacroPDP.loadPdpPage(driver, nonGiftableGame);
//        giftingOptionAvailable = pdpHeroCTA.verifyBuyDropdownPurchaseAsGiftItemAvailable();
//        if (pdpPageLoaded && !giftingOptionAvailable) {
//            logPass(String.format("Verified PDP page for non-giftable '%s' does not have 'Purchase As Gift' option at the 'Buy' button dropdown", nonGiftableGameName));
//        } else {
//            logFailExit(String.format("Failed: PDP page for non-giftable '%s' has 'Purchase As Gift' option at the 'Buy' button dropdown", nonGiftableGameName));
//        }

        softAssertAll();
    }
}
