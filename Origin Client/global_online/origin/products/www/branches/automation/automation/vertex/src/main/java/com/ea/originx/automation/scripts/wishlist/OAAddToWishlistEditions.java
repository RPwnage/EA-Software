package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
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
 * Test to add higher editions of a game to the wishlist for a non subscriber
 * user
 *
 * @author nvarthakavi
 */
public class OAAddToWishlistEditions extends EAXVxTestTemplate {

    @TestRail(caseId = 11710)
    @Test(groups = {"wishlist", "full_regression", "int_only"})
    public void testAddToWishlistEditions(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo higherEditionEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        String entitlementName = higherEditionEntitlement.getName();

        logFlowPoint("Launch Origin and login as newly registered account."); // 1
        logFlowPoint("Navigate to GDP of a higher edition entitlement and add it to wishlist."); // 2
        logFlowPoint("Verify an animated heart and the message 'Added to wishlist' appears"); // 3
        logFlowPoint("Click on 'View Wishlist' link and verify it navigates to the wishlist page and the entitlement "
                + entitlementName + " is on the wishlist "); // 4
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        logPassFail(MacroWishlist.addToWishlist(driver, higherEditionEntitlement), true);

        // 3
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        logPassFail(offerSelectionPage.verifyOnWishlist(), true);

        // 4
        offerSelectionPage.clickViewWishlistLink();
        boolean isOnWishlist = new ProfileWishlistTab(driver).verifyTilesExist(higherEditionEntitlement.getOfferId());
        logPassFail(isOnWishlist, true);

        softAssertAll();
    }
}
