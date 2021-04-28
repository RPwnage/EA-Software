package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlistMultipleEdition;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a multiple edition game to the wishlist for a non-subscriber user
 *
 * @author cdeaconu
 */
public class OAAddToWishlistMultipleEditionSignedUser extends OAAddToWishlistMultipleEdition{
    
    @TestRail(caseId = 3068176)
    @Test(groups = {"gdp", "wishlist", "full_regression"})
    public void testAddToWishlistMultipleEditionSignedUser(ITestContext context) throws Exception {
        testAddToWishlistMultipleEdition(context, OAAddToWishlistMultipleEdition.TEST_TYPE.SIGNED_IN);
    }
}