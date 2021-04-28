package com.ea.originx.automation.scripts.checkout.parameterized;

import com.ea.originx.automation.scripts.checkout.OAPurchaseEntitlement;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests for OAPurchaseEntitlement for Origin-Project Tests
 *
 * @author nvarthakavi
 */
public class OAPurchaseEntitlementOrigin extends OAPurchaseEntitlement {

    @TestRail(caseId = 11878)
    @Test(groups = {"checkout", "full_regression"})
    public void testPurchaseEntitlementOrigin(ITestContext context) throws Exception {
        testPurchaseEntitlement(context);
    }
}
