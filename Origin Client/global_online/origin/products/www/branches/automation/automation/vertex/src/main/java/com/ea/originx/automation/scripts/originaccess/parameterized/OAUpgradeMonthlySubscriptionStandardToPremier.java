package com.ea.originx.automation.scripts.originaccess.parameterized;

import com.ea.originx.automation.scripts.originaccess.OAUpgradeSubscriptionTierStandardToPremier;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the flow of upgrading from a basic monthly subscription to a monthly
 * premier subscription
 *
 * @author cbalea
 */
public class OAUpgradeMonthlySubscriptionStandardToPremier extends OAUpgradeSubscriptionTierStandardToPremier {

    @TestRail(caseId = 2997365)
    @Test(groups = {"full_regression", "origin_access"})
    public void testUpgradeYearlySubscriptionStandardToPremier(ITestContext context) throws Exception {
        testUpgradeSubscriptionTierStandardToPremier(context, false);
    }
}
