package com.ea.originx.automation.scripts.wishlist.parameterized;

import com.ea.originx.automation.scripts.wishlist.OAAddToWishlist;
import com.ea.vx.annotations.TestRail;

import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to add a virtual currency to the wishlist
 *
 * @author nvarthakavi
 */
public class OAAddToWishlistVirtualCurrency extends OAAddToWishlist {

    @TestRail(caseId = 11716)
    @Test(groups = {"wishlist", "full_regression"})
    public void testAddToWishlistVirtualCurrency(ITestContext context) throws Exception {
        testAddToWishlist(context, TEST_TYPE.VIRTUAL_CURRENCY);
    }
}
