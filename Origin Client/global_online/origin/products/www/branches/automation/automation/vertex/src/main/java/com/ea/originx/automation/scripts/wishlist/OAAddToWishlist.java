package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests to Add different entitlements to Wishlist
 *
 * @author nvarthakavi
 */
public class OAAddToWishlist extends EAXVxTestTemplate {

    public enum TEST_TYPE {

        VIRTUAL_CURRENCY_BUNDLE,
        VIRTUAL_CURRENCY,
        THIRD_PARTY,
        NON_SUBSCRIBER,
        SUBSCRIBER
    }

    public void testAddToWishlist(ITestContext context, OAAddToWishlist.TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManager.getInstance().createNewThrowAwayAccount();

        EntitlementInfo entitlement;
        String entitlementName;
        String extraText_1 = "";
        String extraText_2 = "";
        String extraText_3 = "";
        String extraText_4 = "";

        switch (type) {
            case VIRTUAL_CURRENCY_BUNDLE:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.FIFA_18_RONALDO);
                entitlementName = entitlement.getName();
                break;
            case VIRTUAL_CURRENCY:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PVZ2_63000_COIN_PACK);
                entitlementName = entitlement.getName();
                break;
            case THIRD_PARTY:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
                entitlementName = entitlement.getName();
                break;
            case NON_SUBSCRIBER:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
                entitlementName = entitlement.getName();
                break;
            case SUBSCRIBER:
                extraText_1 = " and purchase Origin Access";
                extraText_2 = " , verify the discount message shown ";
                extraText_3 = " and 'Included with Origin Access' message";
                extraText_4 = "verify that there is Play and Buy CTA for the vault entitlement";
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
                entitlementName = entitlement.getName();
                break;
            default:
                throw new RuntimeException("Unexpected test type " + type);
        }

        logFlowPoint("Launch and login as a new account " + user.getUsername() + extraText_1); // 1
        logFlowPoint("Navigate to the" + entitlementName + "'s GDP Page through vault page "
                + "and add the entitlement to the wishlist" + extraText_2); // 2
        logFlowPoint("Verify on GDP page that an animated heart, 'View full wishlist' link, "
                + "'Added to your wishlist' message" + extraText_3 + " appear"); // 3
        logFlowPoint("Click on 'View full wishlist' link and verify it navigates to the wishlist page"
                + " and the entitlement " + entitlementName + " is on the wishlist " + extraText_4); // 4

        //1
        final WebDriver driver = startClientObject(context, client);
        boolean isLogin = MacroLogin.startLogin(driver, user);
        if (type == TEST_TYPE.SUBSCRIBER) {
            isLogin &= MacroOriginAccess.purchaseOriginAccess(driver);
        }
        logPassFail(isLogin, true);

        //2
        boolean isAddingToWishlist = MacroWishlist.addToWishlist(driver, entitlement.getParentName(),
                entitlement.getOcdPath(), entitlement.getPartialPdpUrl());
        if (type == TEST_TYPE.SUBSCRIBER) {
            OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
            isAddingToWishlist &= offerSelectionPage.verifyOriginAccessDiscountIsVisible();
        }
        logPassFail(isAddingToWishlist, true);

        //3
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        boolean isAddedToWishlist = MacroWishlist.
                verifyAddedToWishlist(driver, entitlement, gdpActionCTA.verifyGDPHeaderReached());
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        if (type == TEST_TYPE.SUBSCRIBER) {
            isAddedToWishlist &= gdpHeroActionDescriptors.
                    verifyFirstInfoBlockTextMatchesExpected(GDPHeroActionDescriptors.INCLUDED_WITH_ORIGIN_ACCESS_BADGE);
        }
        logPassFail(isAddedToWishlist, true);

        //4
        gdpActionCTA.clickViewFullWishlistLink();
        boolean isOnWishlist = new ProfileHeader(driver).verifyWishlistActive();
        boolean isEntitlementOnWishlist = MacroWishlist.verifyTilesExist(driver, entitlement.getOfferId());
        boolean isBuyAndPlayCTAShown = true;
        if (type == TEST_TYPE.SUBSCRIBER) {
            isBuyAndPlayCTAShown = MacroWishlist.verifyBuyAndOriginAccessCTAs(driver, entitlement);
        }
        logPassFail(isOnWishlist && isEntitlementOnWishlist && isBuyAndPlayCTAShown, true);

        softAssertAll();
    }
}
