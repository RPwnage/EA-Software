package com.ea.originx.automation.scripts.loginregistration.parameterized;

import com.ea.originx.automation.scripts.loginregistration.OAPasswordChange;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OAPasswordChange - Simple password change
 * parameters: <br>
 * testOffline: false <br>
 * testName: name of this class <br>
 *
 * @author jmitterteiner, palui
 */
public class OAOnlineModeChangePassword extends OAPasswordChange {

    @TestRail(caseId = 10103)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testOnlineModeChangePassword(ITestContext context) throws Exception {
        testPasswordChange(context, false, this.getClass().getName());
    }
}
