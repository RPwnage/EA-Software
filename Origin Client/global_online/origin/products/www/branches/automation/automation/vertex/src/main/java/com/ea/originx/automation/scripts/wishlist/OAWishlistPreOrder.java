package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import static com.ea.originx.automation.lib.resources.games.EntitlementId.GDP_PRE_RELEASE_TEST_OFFER01_STANDARD;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
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
 * Test adding pre-order entitlement to Wishlist
 *
 * @author palui
 */
public class OAWishlistPreOrder extends EAXVxTestTemplate {

    @TestRail(caseId = 11714)
    @Test(groups = {"wishlist", "full_regression"})
    public void testWishlistPreOrder(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount user = AccountManager.getInstance().createNewThrowAwayAccount();
        final String entitlementOfferID = Sims4.SIMS4_PREORDER_TEST_DLC_OFFER_ID;
        final String entitementPartialGdpUrl = Sims4.SIMS4_PREORDER_TEST_DLC_NAME_PARTIAL_PDP_URL;
        final String entitementName = Sims4.SIMS4_PREORDER_TEST_DLC_NAME;

        logFlowPoint("Log into Origin as newly registered user"); //1
        logFlowPoint("Navigate to the GDP of an entitlement currently on pre-order"); //2
        logFlowPoint("Add entitlement to wishlist and verify entitlement is added to wishlist"); //3
        logFlowPoint("Click on 'View full wishlist' link and verify entitlement is in the wishlist"); //4

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        logPassFail(MacroWishlist.addToWishlist(driver, entitementName, entitlementOfferID, entitementPartialGdpUrl), true);

        //3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        logPassFail(gdpActionCTA.verifyOnWishlist(), true);

        //4
        gdpActionCTA.clickViewFullWishlistLink();
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        profileWishlistTab.waitForWishlistTilesToLoad();
        logPassFail(profileWishlistTab.verifyTilesExist(entitlementOfferID), true);

        softAssertAll();
    }
}
