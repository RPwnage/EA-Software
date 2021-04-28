package com.ea.originx.automation.scripts.feature.originaccess.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.scripts.feature.originaccess.OAUpgradeEntitlement;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the upgrade flow for Battlefield 4 Standard to Battlefield 4 Premium.
 *
 * @author glivingstone
 */
public class OAUpgradeEntitlementBattlefield extends OAUpgradeEntitlement {

    @TestRail(caseId = 1016694)
    @Test(groups = {"release_smoke", "originaccess_smoke", "originaccess", "client_only", "allfeaturesmoke"})
    public void testUpgradeEntitlementBattlefield(ITestContext context) throws Exception {
        testUpgradeEntitlement(context, EntitlementId.BF4_STANDARD, EntitlementId.BF4_PREMIUM);
    }
}
