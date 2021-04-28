package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests a vault game can be added to 'Wishlist' and has options to be Added to
 * Game Library, bought or removed from 'Wishlist'
 *
 * @author cbalea
 */
public class OAWishlistVaultGame extends EAXVxTestTemplate {

    @TestRail(caseId = 11721)
    @Test(groups = {"wishlist", "full_regression", "int_only"})
    public void testWishlistSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount wishlistAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final EntitlementInfo vaultGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        final String vaultGameOfferId = vaultGame.getOfferId();

        logFlowPoint("Log into Origin as newly registered account"); // 1
        logFlowPoint("Purchase Origin Access"); // 2
        logFlowPoint("Navigate to vault game GDP"); // 3
        logFlowPoint("Select the arrow next to the 'Add to Library' CTA , select 'Add to Wishlist' and verify game has been added to wishlist"); // 4
        logFlowPoint("Click 'View Full Wishlist' and verify game appears in Wishlist on 'Profile'"); // 5
        logFlowPoint("Verify 'Buy' CTA appears on Wishlist"); // 6
        logFlowPoint("Verify that the user can add the game to the library"); // 7
        logFlowPoint("Verify game can be removed from Wishlist"); // 8

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, wishlistAccount), true);
        
        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        
        // 3
        logPassFail(MacroGDP.loadGdpPage(driver, vaultGame), true);

        // 4
        GDPActionCTA gDPActionCTA = new GDPActionCTA(driver);
        gDPActionCTA.selectDropdownAddToWishlist();
        logPassFail(gDPActionCTA.verifyOnWishlist(), true);

        // 5
        gDPActionCTA.clickViewFullWishlistLink();
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        logPassFail(profileWishlistTab.verifyTilesExist(vaultGameOfferId), false);

        // 6
        logPassFail(profileWishlistTab.verifyBuyButtonsExist(vaultGame), false);

        // 7
        logPassFail(profileWishlistTab.verifyPlayAccessButtonsExist(vaultGame), false);

        // 8
        logPassFail(profileWishlistTab.verifyRemoveFromWishlistLinkExistForAllItems(), true);

        softAssertAll();
    }
}
