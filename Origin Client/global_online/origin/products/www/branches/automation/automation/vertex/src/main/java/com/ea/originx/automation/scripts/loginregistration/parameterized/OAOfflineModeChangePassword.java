package com.ea.originx.automation.scripts.loginregistration.parameterized;

import com.ea.originx.automation.scripts.loginregistration.OAPasswordChange;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OAPasswordChange - Password change while in
 * offline mode <br>
 * parameters: <br>
 * testOffline: true <br>
 * testName: name of this class <br>
 *
 * @author palui
 */
public class OAOfflineModeChangePassword extends OAPasswordChange {

    @TestRail(caseId = 39662)
    @Test(groups = {"loginregistration", "client_only", "full_regression"})
    public void testOfflineModeChangePassword(ITestContext context) throws Exception {
        testPasswordChange(context, true, this.getClass().getName());
    }
}
