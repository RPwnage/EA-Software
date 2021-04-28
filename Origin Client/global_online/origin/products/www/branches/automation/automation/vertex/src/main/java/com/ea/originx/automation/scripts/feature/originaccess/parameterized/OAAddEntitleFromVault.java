package com.ea.originx.automation.scripts.feature.originaccess.parameterized;

import com.ea.originx.automation.scripts.feature.originaccess.OAEntitleFromVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests adding vault entitlement to game library
 *
 * @author rocky
 */
public class OAAddEntitleFromVault extends OAEntitleFromVault {

    @TestRail(caseId = 1016693)
    @Test(groups = {"release_smoke", "originaccess"})
    public void testAddEntitleFromVault(ITestContext context) throws Exception {
        testEntitleFromVault(context, TEST_ACTION.ADD, TEST_PAGE.VAULT_PAGE);
    }
}
