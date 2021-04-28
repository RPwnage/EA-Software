package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNavigation;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the appearance and addition of Vault titles on Wishlist for an expired
 * Origin Access subscriber
 *
 * @author cbalea
 */
public class OAWishlistExpiredSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 11720)
    @Test(groups = {"wishlist", "client_only", "full_regression", "int_only"})
    public void testWishlistExpiredSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo vaultEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        String vaultEntitlementOfferID = vaultEntitlement.getOfferId();

        final AccountTags tag = AccountTags.SUBSCRIPTION_EXPIRED_OWN_ENT;
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(tag);

        logFlowPoint("Log into an account that has an expired Subscription and a game entitled through Origin Access"); //1
        logFlowPoint("Navigate to the 'Game Library' and verify that the entitled game appears"); //2
        logFlowPoint("Navigate to the GDP of the vault entitlement"); // 3
        logFlowPoint("Click 'Add to Wishlist' under the CTA drop down list and verify entitlement has been added to wishlist"); //4
        logFlowPoint("Reach 'User Profile', click on wishlist tab and verify entitlement appears in wishlist"); //5
        logFlowPoint("Verify that the purchase 'CTA' appears on the wishlist tile"); //6
        logFlowPoint("Verify that the 'Remove from wishlist' CTA is displayed"); // 7

        final WebDriver userDriver = startClientObject(context, client);

        // 1
        logPassFail(MacroLogin.startLogin(userDriver, userAccount), true);

        // 2
        new MacroNavigation(userDriver).toMyGameLibraryPage();
        logPassFail(new GameTile(userDriver, vaultEntitlementOfferID).verifyViolatorStatingMembershipIsExpired(), true);

        // 3
        MacroWishlist.navigateToWishlist(userDriver);
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(userDriver);
        profileWishlistTab.removeAllWishlistItems();
        logPassFail(MacroGDP.loadGdpPage(userDriver, vaultEntitlement), true);

        // 4
        GDPActionCTA gdpActionCTA = new GDPActionCTA(userDriver);
        gdpActionCTA.selectDropdownAddToWishlist();
        logPassFail(gdpActionCTA.verifyOnWishlist(), true);

        // 5
        MacroWishlist.navigateToWishlist(userDriver);
        logPassFail(profileWishlistTab.verifyProductTitlesExist(vaultEntitlement), false);

        // 6
        logPassFail(profileWishlistTab.verifyBuyButtonsExist(vaultEntitlement), false);

        // 7
        logPassFail(profileWishlistTab.verifyRemoveFromWishlistLinkExistForAllItems(), false);

        softAssertAll();
    }
}
