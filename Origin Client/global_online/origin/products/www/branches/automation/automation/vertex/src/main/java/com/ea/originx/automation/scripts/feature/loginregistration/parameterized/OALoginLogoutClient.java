package com.ea.originx.automation.scripts.feature.loginregistration.parameterized;

import com.ea.originx.automation.scripts.feature.loginregistration.OALoginLogout;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the logging in and logging out on Client
 *
 * @author sbentley
 */
public class OALoginLogoutClient extends OALoginLogout {

    @TestRail(caseId = 1016748)
    @Test(groups = {"loginregistration", "loginregistration_smoke", "client_only", "full_regression", "allfeaturesmoke", "release_smoke"})
    public void OALoginLogoutClient(ITestContext context) throws Exception {
        OALoginLogout(context, false);
    }
}
