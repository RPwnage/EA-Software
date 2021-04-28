package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.scripts.client.OARedeemDialog;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OARedeemDialog - DiP large (underage account)
 *
 * @author jdickens
 */
public class OARedeemDipLargeUnderage extends OARedeemDialog {

    @TestRail(caseId =  702052)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testRedeemDipLarge(ITestContext context) throws Exception {
        testRedeemDialog(context, EntitlementId.DIP_LARGE, true);
    }
}
