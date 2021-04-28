package com.ea.originx.automation.scripts.originaccess.parameterized;

import com.ea.originx.automation.scripts.originaccess.OAUpgradeSubscriptionTierStandardToPremier;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the flow of upgrading from a basic yearly subscription to a yearly
 * premier subscription
 *
 * @author cbalea
 */
public class OAUpgradeYearlySubscriptionStandardToPremier extends OAUpgradeSubscriptionTierStandardToPremier {
    @TestRail(caseId = 2997365)
    @Test(groups = {"full_regression", "origin_access"})
    public void testUpgradeYearlySubscriptionStandardToPremier(ITestContext context) throws Exception {
        testUpgradeSubscriptionTierStandardToPremier(context, true);
    }
}
