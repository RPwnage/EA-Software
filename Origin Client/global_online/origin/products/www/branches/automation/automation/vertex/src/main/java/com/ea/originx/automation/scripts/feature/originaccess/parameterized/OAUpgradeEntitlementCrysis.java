package com.ea.originx.automation.scripts.feature.originaccess.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.scripts.feature.originaccess.OAUpgradeEntitlement;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the upgrade flow for Crysis to Crysis Digital Deluxe.
 *
 * @author glivingstone
 */
public class OAUpgradeEntitlementCrysis extends OAUpgradeEntitlement {

    @Test(groups = {"gamelibrary", "originaccess_smoke", "originaccess", "client_only", "allfeaturesmoke"})
    public void testUpgradeEntitlementCrysis(ITestContext context) throws Exception {
        testUpgradeEntitlement(context, EntitlementId.CRYSIS3, EntitlementId.CRYSIS3_HUNTER);
    }
}
