package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Add an entitlement to the 'Wishlist' and check if it is visible in the
 * 'Wishlist' tab
 *
 * @author cdeaconu
 */
public class OAInfoBlockOnWishlist extends EAXVxTestTemplate{
    
    @TestRail(caseId = 3064048)
    @Test(groups = {"wishlist", "full_regression"})
    public void testInfoBlockOnWishlist(ITestContext context) throws Exception{
        
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.STAR_WARS_BATTLEFRONT);
        final String entitlementOfferId = entitlement.getOfferId();
         
        logFlowPoint("Log into Origin as newly registered account."); // 1
        logFlowPoint("Navigate to GDP of a released entitlement."); // 2
        logFlowPoint("Add the entitlement to wishlist and verify a red heart icon is showing."); // 3
        logFlowPoint("Verify the 'Added to your wishlist View full wishlist' text is showing on the right of the icon."); // 4
        logFlowPoint("Verify the 'View full wishlist' in the text is a link."); // 5
        logFlowPoint("Click on the 'View full wishlist' link and verify the page redirects to the wishlist tab on the account's profile page"); // 6
        
        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);
        
        // 3
        gdpActionCTA.selectDropdownAddToWishlist();
        boolean isHeartIconVisible = gdpActionCTA.verifyWishlistHeartIconVisible();
        boolean isHeartIconRed = gdpActionCTA.verifyWishlistHeartIconRed();
        logPassFail(isHeartIconVisible && isHeartIconRed, false);
        
        // 4
        boolean isWishlistTextVisible = gdpActionCTA.verifyOnWishlistMessageText();
        boolean isWishlistTextRightOfIcon = gdpActionCTA.verifyWishlistTextIsRightOfIcon();
        logPassFail(isWishlistTextVisible && isWishlistTextRightOfIcon, false);
        
        // 5
        logPassFail(gdpActionCTA.verifyViewWishlistLinkText(), true);
        
        // 6
        gdpActionCTA.clickViewFullWishlistLink();
        boolean isTileVisible = new ProfileWishlistTab(driver).verifyTilesExist(entitlementOfferId);
        logPassFail(isTileVisible, true);
        
        softAssertAll();
    }
}