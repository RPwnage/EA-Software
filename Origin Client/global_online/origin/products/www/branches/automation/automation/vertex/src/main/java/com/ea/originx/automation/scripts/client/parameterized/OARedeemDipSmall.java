package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.scripts.client.OARedeemDialog;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OARedeemDialog - DiP small
 * 
 * @author palui
 */
public class OARedeemDipSmall extends OARedeemDialog {

    @Test(groups = {"client", "client_only"})
    public void testRedeemDipSmall(ITestContext context) throws Exception {
        testRedeemDialog(context, EntitlementId.DIP_SMALL, false);
    }
}
