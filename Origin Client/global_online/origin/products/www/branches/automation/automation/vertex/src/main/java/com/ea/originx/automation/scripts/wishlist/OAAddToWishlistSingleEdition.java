package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
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
 * Add an entitlement to the 'Wishlist' of a non-subscriber account and check if
 * it is visible in the 'Wishlist' tab
 *
 * @author cdeaconu
 */
public class OAAddToWishlistSingleEdition extends EAXVxTestTemplate{
    
    public enum TEST_TYPE {
        NON_SUBSCRIBER,
        ANONYMOUS_USER
    }
    
    public void testAddToWishlistSingleEdition(ITestContext context, TEST_TYPE type) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BLACK_MIRROR);
        final String entitlementOfferId = entitlement.getOfferId();
        
        if(type == TEST_TYPE.NON_SUBSCRIBER) {
            logFlowPoint("Log into Origin as newly registered account: " + userAccount.getUsername()); // 1
        }
        logFlowPoint("Navigate to GDP of a single edition vault entitlement " +
                "and select 'Add to wishlist' option in the drop-down menu."); // 2
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logFlowPoint("Go to origin as an anonymous user."); // 3
        }
        logFlowPoint("Verify the added to wishlist info block is showing."); // 4
        logFlowPoint("Navigate to the wishlist tab in the Profile page " +
                "and verify the offer is on the wishlist."); // 5
        
        WebDriver driver = startClientObject(context, client);

        // 1
        if (type == TEST_TYPE.NON_SUBSCRIBER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
        
        // 2
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        gdpActionCTA.selectDropdownAddToWishlist();
        
        // 3
        if (type == TEST_TYPE.ANONYMOUS_USER) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }
       
        // 4
        logPassFail(gdpActionCTA.verifyOnWishlist(), true);
        
        // 5
        MacroWishlist.navigateToWishlist(driver);
        boolean isTileVisible = new ProfileWishlistTab(driver).verifyTilesExist(entitlementOfferId);
        logPassFail(isTileVisible, true);
        
        softAssertAll();
    }
}