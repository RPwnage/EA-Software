package com.ea.originx.automation.scripts.ogd.parameterized;

import com.ea.originx.automation.scripts.ogd.OAAddToWishlistOGD;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that a discount is applied to on sale wishlisted games
 *
 * @author jmittertreiner
 */
public class OAAddToWishlistOGDSale extends OAAddToWishlistOGD {

    @TestRail(caseId = 11709)
    @Test(groups = {"ogd", "wishlist", "full_regression"})
    public void testAddToWishlistOGDAccess(ITestContext c) throws Exception {
        testAddToWishlistOGD(c, TEST_TYPE.ON_SALE);
    }
}
