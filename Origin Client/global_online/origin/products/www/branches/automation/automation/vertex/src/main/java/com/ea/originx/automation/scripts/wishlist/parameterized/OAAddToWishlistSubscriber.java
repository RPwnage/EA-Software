package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlist;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a vault game to the wishlist for a subscriber user
 *
 * @author nvarthakavi
 */
public class OAAddToWishlistSubscriber extends OAAddToWishlist {

    @TestRail(caseId = 11711)
    @Test(groups = {"wishlist", "full_regression"})
    public void testAddToWishlistSubscriber(ITestContext context) throws Exception {
        testAddToWishlist(context, TEST_TYPE.SUBSCRIBER);
    }
}
