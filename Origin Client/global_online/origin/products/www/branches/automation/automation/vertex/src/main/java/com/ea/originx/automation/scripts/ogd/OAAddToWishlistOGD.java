package com.ea.originx.automation.scripts.ogd;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.ExtraContentGameCard;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test adding to the wishlist through the Origin Game Details page
 *
 * @author jmittertreiner
 */
public class OAAddToWishlistOGD extends EAXVxTestTemplate {

    public enum TEST_TYPE {

        THIRD_PARTY,
        ACCESS,
        ON_SALE,
        BASIC
    }

    public void testAddToWishlistOGD(ITestContext c, TEST_TYPE type) throws Exception {

        final OriginClient client = OriginClientFactory.create(c);

        UserAccount user = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        EntitlementInfo entitlement;
        String dlcOfferId;
        switch (type) {
            case ACCESS:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_DIGITAL_DELUXE);
                dlcOfferId = Sims4.SIMS4_DLC_BACKYARD_OFFER_ID;
                break;
            case ON_SALE:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
                dlcOfferId = Sims4.SIMS4_DISCOUNT_TEST_DLC_OFFER_ID;
                break;
            case THIRD_PARTY:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THIS_WAR_OF_MINE);
                dlcOfferId = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.THE_LITTLE_ONES).getOfferId();
                break;
            case BASIC:
                entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF_BAD_COMPANY_2);
                dlcOfferId = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF_BAD_COMPANY_2_VIETNAM).getOfferId();
                break;
            default:
                throw new RuntimeException("Unexpected test type " + type);
        }

        logFlowPoint("Launch and login as a new account "
                + user.getUsername() + "and purchase " + entitlement.getName()
                + (type == TEST_TYPE.ACCESS ? " and Origin Access" : "")); // 1
        logFlowPoint("Navigate to the Game Library and go to the Extra content page of " + entitlement.getName()); // 2
        logFlowPoint("Add the DLC to the Wishlist and verify that an animated heart and a message appears"); // 3
        logFlowPoint("Click on the 'View full wishlist' link and verify the DLC add-on in wishlist" + (type != TEST_TYPE.THIRD_PARTY && type != TEST_TYPE.BASIC
                ? " with discount information reflected on the tile."
                : "")); // 4

        // 1
        final WebDriver driver = startClientObject(c, client);
        boolean isLoggedIn  = MacroLogin.startLogin(driver, user);
        boolean isBaseGameEntitled = false;
        if (type == TEST_TYPE.ACCESS) {
            MacroOriginAccess.purchaseOriginAccess(driver);
            MacroGDP.loadGdpPage(driver, entitlement);
            isBaseGameEntitled = MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver);
        } else {
            isBaseGameEntitled = MacroPurchase.purchaseEntitlement(driver, entitlement);
        }
        logPassFail(isLoggedIn && isBaseGameEntitled, true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlement.getOfferId());
        GameSlideout gameSlideout = gameTile.openGameSlideout();
        if (type == TEST_TYPE.ACCESS){
            new MainMenu(driver).clickMaximizeButton();
            sleep(5000); // wait for Origin client to maximize
            gameSlideout.clickStuffPackTab();
        } else {
            gameSlideout.clickExtraContentTab();
            sleep(5000); // wait for content to load
        }
        logPassFail(gameSlideout.verifyExtraContentInExtraContentTab(), true);
        
        // 3
        ExtraContentGameCard dlcCard = new ExtraContentGameCard(driver, dlcOfferId);
        dlcCard.scrollToGameCard();
        dlcCard.selectBuyDropdownAddToWishlist();
        logPassFail(dlcCard.verifyWishlistHeartIconVisible() && dlcCard.verifyOnWishlistText() && dlcCard.verifyViewWishlistLinkText(), false);

        // 4
        dlcCard.clickViewWishlistLink();
        Waits.pollingWait(() -> new ProfileWishlistTab(driver).verifyTilesExist(dlcOfferId));
        WishlistTile tile = new ProfileWishlistTab(driver).getWishlistTile(dlcOfferId);
        // This check is to make sure when a Test Type is either Third Party or Basic the Tile should not be discounted other cases it should be discounted
        logPassFail(tile != null && ((type == TEST_TYPE.THIRD_PARTY || type == TEST_TYPE.BASIC) != tile.isDiscounted()), false);

        softAssertAll();
    }
}
