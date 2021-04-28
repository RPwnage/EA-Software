package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlistSingleEdition;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a single edition game to the wishlist for a non subscriber user
 * (browser only)
 *
 * @author cdeaconu
 */
public class OAAddToWishlistSingleEditionAnonymous extends OAAddToWishlistSingleEdition{
    
    @TestRail(caseId = 3064509)
    @Test(groups = {"gdp", "wishlist", "full_regression", "browser_only"})
    public void testAddToWishlistSingleEditionAnonymous(ITestContext context) throws Exception {
        testAddToWishlistSingleEdition(context, OAAddToWishlistSingleEdition.TEST_TYPE.ANONYMOUS_USER);
    }
}
