package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.scripts.client.OARedeemDialog;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OARedeemDialog - Non-DiP large
 * 
 * @author palui
 */
public class OARedeemNonDipLarge extends OARedeemDialog {

    @Test(groups = {"client", "client_only"})
    public void testRedeemNonDipLarge(ITestContext context) throws Exception {
        testRedeemDialog(context, EntitlementId.NON_DIP_LARGE, false);
    }
}
