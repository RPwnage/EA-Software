package com.ea.originx.automation.scripts.feature.loginregistration.parameterized;

import com.ea.originx.automation.scripts.feature.loginregistration.OALoginLogout;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the logging in and logging out on Web
 *
 * @author sbentley
 */
public class OALoginLogoutWeb extends OALoginLogout {

    @TestRail(caseId = 1016680)
    @Test(groups = {"loginregistration_smoke", "loginregistration", "browser_only", "allfeaturesmoke", "release_smoke"})
    public void OALoginLogoutWeb(ITestContext context) throws Exception {
        OALoginLogout(context, true);
    }
}
