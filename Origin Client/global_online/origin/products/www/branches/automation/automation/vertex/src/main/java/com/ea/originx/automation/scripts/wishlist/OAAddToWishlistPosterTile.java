package com.ea.originx.automation.scripts.wishlist;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.store.DealsPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreGameTile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Add to Wishlist option that appears when hovering over a game tile
 *
 * @author cvanichsarn
 */
public class OAAddToWishlistPosterTile extends EAXVxTestTemplate {

    @TestRail(caseId = 158775)
    @Test(groups = {"wishlist", "client_only", "full_regression"})
    public void testAddToWishlistPosterTile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount user = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Log into Origin with a newly created account"); //1
        logFlowPoint("Navigate to the Deals Page and hover over a poster tile to verify the 'Add to Wishlist' option has a hollow heart icon"); //2
        logFlowPoint("Click on the 'Add to Wishlist' option to verify it switches to 'Added to Wishlist' with a filled heart icon"); //3
        logFlowPoint("Navigate to the user wishlist to verify the offer has been added"); //4
        logFlowPoint("Navigate to the Deals Page and click on the 'On My Wishlist' option for the same offer to remove it from the wishlist "
                + "and to verify it switches to 'Add to Wishlist' with a hollow heart icon"); //5
        logFlowPoint("Navigate to the user wishlist to verify the offer has been removed"); //6

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        DealsPage dealPage = navBar.gotoDealsPage();
        StoreGameTile storeGameTile = dealPage.getFirstStoreGameTile();
        String offerId = storeGameTile.getOfferId();
        logPassFail(storeGameTile.verifyAddToWishlistVisible(), true);

        //3
        storeGameTile.clickAddToWishlist();
        logPassFail(storeGameTile.verifyRemoveFromWishlistVisible(), true);

        //4
        MacroWishlist.navigateToWishlist(driver);
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        logPassFail(profileWishlistTab.verifyTilesExist(offerId), true);

        //5
        navBar.gotoDealsPage();
        storeGameTile.clickRemoveFromWishlist();
        logPassFail(storeGameTile.verifyAddToWishlistVisible(), true);

        //6
        MacroWishlist.navigateToWishlist(driver);
        logPassFail(profileWishlistTab.isEmptyWishlist(driver), true);

        softAssertAll();
    }
}
