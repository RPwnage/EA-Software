package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlist;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a third party game to the wishlist
 *
 * @author nvarthakavi
 */
public class OAAddToWishlistThirdParty extends OAAddToWishlist {

    @TestRail(caseId = 11715)
    @Test(groups = {"wishlist", "full_regression"})
    public void testAddToWishlistThirdParty(ITestContext context) throws Exception {
        testAddToWishlist(context, TEST_TYPE.THIRD_PARTY);
    }
}
