package com.ea.originx.automation.scripts.feature.originaccess.parameterized;

import com.ea.originx.automation.scripts.feature.originaccess.OAEntitleFromVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests removing vault entitlement to game library
 *
 * @author rocky
 */
public class OARemoveEntitleFromVault extends OAEntitleFromVault {

    @TestRail(caseId = 1016729)
    @Test(groups = {"services_smoke", "originaccess", "originaccess_smoke", "allfeaturesmoke", "int_only"})
    public void testRemoveEntitleFromVault(ITestContext context) throws Exception {
        testEntitleFromVault(context, TEST_ACTION.REMOVE, TEST_PAGE.GDP_PAGE);
    }
}
