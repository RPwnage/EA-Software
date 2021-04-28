package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlist;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a vault game to the wishlist for a non subscriber user
 *
 * @author nvarthakavi
 */
public class OAAddToWishlistNonSubscriber extends OAAddToWishlist {

    @TestRail(caseId = 11712)
    @Test(groups = {"wishlist", "full_regression"})
    public void testAddToWishlistNonSubscriber(ITestContext context) throws Exception {
        testAddToWishlist(context, TEST_TYPE.NON_SUBSCRIBER);
    }
}
