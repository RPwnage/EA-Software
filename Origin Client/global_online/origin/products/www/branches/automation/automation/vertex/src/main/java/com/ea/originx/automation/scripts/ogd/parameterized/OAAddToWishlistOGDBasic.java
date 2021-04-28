package com.ea.originx.automation.scripts.ogd.parameterized;

import com.ea.originx.automation.scripts.ogd.OAAddToWishlistOGD;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that a basic entitlement's dlc can be added to the wishlist through OGD
 *
 * @author nvarthakavi
 */
public class OAAddToWishlistOGDBasic extends OAAddToWishlistOGD {

    @TestRail(caseId = 11705)
    @Test(groups = {"ogd", "wishlist", "full_regression"})
    public void testAddToWishlistOGDBasic(ITestContext c) throws Exception {
        testAddToWishlistOGD(c, TEST_TYPE.BASIC);
    }
}
