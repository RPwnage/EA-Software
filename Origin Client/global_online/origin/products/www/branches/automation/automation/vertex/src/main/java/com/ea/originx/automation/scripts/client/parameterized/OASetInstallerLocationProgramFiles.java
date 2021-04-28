package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.scripts.client.OAInvalidInstallerLocation;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OAInvalidInstallerLocation - C:\Program Files\
 *
 * @author micwang
 */
public class OASetInstallerLocationProgramFiles extends OAInvalidInstallerLocation {

    @TestRail(caseId = 10075)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testSetInstallerLocationProgramFiles(ITestContext context) throws Exception {
        testInvalidInstallerLocation(context, "C:\\Program Files\\");
    }
}
