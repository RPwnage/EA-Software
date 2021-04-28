package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlistSingleEdition;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a single edition game to the wishlist for a non subscriber user
 *
 * @author cdeaconu
 */
public class OAAddToWishlistSingleEditionSignedUser extends OAAddToWishlistSingleEdition{
    
    @TestRail(caseId = 3064510)
    @Test(groups = {"gdp", "wishlist", "full_regression"})
    public void testAddToWishlistSingleEditionNonSubscriber(ITestContext context) throws Exception {
        testAddToWishlistSingleEdition(context, OAAddToWishlistSingleEdition.TEST_TYPE.NON_SUBSCRIBER);
    }
}
