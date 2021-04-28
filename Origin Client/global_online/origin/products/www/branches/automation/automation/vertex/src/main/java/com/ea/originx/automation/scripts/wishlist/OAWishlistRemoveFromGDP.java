package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.originaccess.AccessInterstitialPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for removing wishlist items from the GDP 'Buy' button dropdown
 *
 * @author palui
 */
public class OAWishlistRemoveFromGDP extends EAXVxTestTemplate {

    @TestRail(caseId = 11702)
    @Test(groups = {"wishlist", "full_regression"})
    public void testWishlistRemoveFromGDP(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount user = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementOcdPath = entitlement.getOcdPath();

        logFlowPoint("Open origin (client or web) and log in"); //1
        logFlowPoint("Go to a GDP for a game that has multiple editions and add entitlement to wishlist"); //2
        logFlowPoint("Navigate to wishlist"); //3
        logFlowPoint("Verify that the item was correctly added to the wishlist"); //4
        logFlowPoint("Return to the OSP for that game, then to the wishlist and verify that item removed " +
                "via the OSP is correctly removed from the wishlist"); //5
        logFlowPoint("Log out and log back in, then return to the wishlist page and verify item remains" +
                " removed from the wishlist"); //6

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        logPassFail(MacroWishlist.addToWishlist(driver, entitlement), true);

        //3
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        logPassFail(profileHeader.verifyWishlistActive(), true);

        //4
        profileHeader.waitForWishlistTabToLoad();
        ProfileWishlistTab wishlistTab = new ProfileWishlistTab(driver);
        boolean isAdded = Waits.pollingWait(() -> wishlistTab.verifyTilesExist(entitlementOfferId));
        logPassFail(isAdded, false);

        //5
        MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).clickGetTheGameCTA();
        AccessInterstitialPage accessInterstitialPage = new AccessInterstitialPage(driver);
        accessInterstitialPage.verifyOSPInterstitialPageReached();
        accessInterstitialPage.clickBuyGameOSPCTA();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        if(offerSelectionPage.verifyOfferSelectionPageReached()) {
            offerSelectionPage.clickPrimaryDropdownAndRemoveFromWishlist(entitlementOcdPath);
        }

        MacroWishlist.navigateToWishlist(driver);
        logPassFail(wishlistTab.isEmptyWishlist(driver), true);

        //6
        miniProfile.selectSignOut();
        sleep(2000); // pause for Login page to appear

        MacroLogin.startLogin(driver, user);
        MacroWishlist.navigateToWishlist(driver);
        logPassFail(wishlistTab.isEmptyWishlist(driver), true);

        softAssertAll();
    }
}