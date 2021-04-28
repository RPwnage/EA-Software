package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlistMultipleEdition;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a multiple edition game to the wishlist for a non-subscriber user
 * (browser only)
 *
 * @author cdeaconu
 */
public class OAAddToWishlistMultipleEditionAnonymous extends OAAddToWishlistMultipleEdition{
    
    @TestRail(caseId = 3068175)
    @Test(groups = {"gdp", "wishlist", "full_regression", "browser_only"})
    public void testAddToWishlistMultipleEditionAnonymous(ITestContext context) throws Exception {
        testAddToWishlistMultipleEdition(context, OAAddToWishlistMultipleEdition.TEST_TYPE.ANONYMOUS_USER);
    }
}