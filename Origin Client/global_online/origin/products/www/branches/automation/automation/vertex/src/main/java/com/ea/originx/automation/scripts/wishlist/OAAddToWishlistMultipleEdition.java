package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Add a multiple edition entitlement to the wishlist for a non-subscriber
 * account and check if it is visible in the 'Wishlist' tab
 *
 * @author cdeaconu
 */
public class OAAddToWishlistMultipleEdition extends EAXVxTestTemplate{
    
    public enum TEST_TYPE {
        SIGNED_IN,
        ANONYMOUS_USER
    }
    
    public void testAddToWishlistMultipleEdition(ITestContext context, TEST_TYPE type) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.TITANFALL_2);
        final String entitlementOfferId = entitlement.getOfferId();
        final String entitlementOcdPath = entitlement.getOcdPath();
        
        if(type == TEST_TYPE.SIGNED_IN) {
            logFlowPoint("Log into Origin as newly registered account."); // 1
        }
        logFlowPoint("Navigate to GDP of a multiple edition vault entitlement "
                + "and select 'Add to Wishlist' option in the drop-down menu."); // 2
        logFlowPoint("Verify the page redirects to 'Offer Selection' page "
                + "and click on 'Add to Wishlist' of any edition."); // 3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Log in as a newly registered user."); // 4
        }
        logFlowPoint("Navigate to the wishlist tab in the Profile page "
                + "and verify the offer is on the wishlist."); // 5
        
        WebDriver driver = startClientObject(context, client);

        // 1
        if (type == TEST_TYPE.SIGNED_IN) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        new GDPActionCTA(driver).selectDropdownAddToWishlist();
        
        // 3
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOfferSelectionPageReached(), true);
        offerSelectionPage.clickPrimaryButton(entitlementOcdPath);
        
        // 4
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 5
        MacroWishlist.navigateToWishlist(driver);
        boolean isTileVisible = new ProfileWishlistTab(driver).verifyTilesExist(entitlementOfferId);
        logPassFail(isTileVisible, true);
        
        softAssertAll();
    }
}