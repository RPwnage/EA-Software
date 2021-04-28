package com.ea.originx.automation.scripts.ogd.parameterized;

import com.ea.originx.automation.scripts.ogd.OAAddToWishlistOGD;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author jmittertreiner
 */
public class OAAddToWishlistOGDThirdParty extends OAAddToWishlistOGD {

    @TestRail(caseId = 11707)
    @Test(groups = {"ogd", "wishlist", "full_regression"})
    public void testAddToWishlistOGDAccess(ITestContext c) throws Exception {
        testAddToWishlistOGD(c, TEST_TYPE.THIRD_PARTY);
    }
}
