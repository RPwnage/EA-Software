package com.ea.originx.automation.scripts.checkout.parameterized;

import com.ea.originx.automation.scripts.checkout.OAPurchaseEntitlement;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests for OAPurchaseEntitlement for Smoke Tests
 *
 * @author nvarthakavi
 */
public class OAPurchaseEntitlementSmoke extends OAPurchaseEntitlement {

    @TestRail(caseId = 1016723)
    @Test(groups = {"server", "checkout", "services_smoke"})
    public void testPurchaseEntitlementSmoke(ITestContext context) throws Exception {
        testPurchaseEntitlement(context);
    }
}
