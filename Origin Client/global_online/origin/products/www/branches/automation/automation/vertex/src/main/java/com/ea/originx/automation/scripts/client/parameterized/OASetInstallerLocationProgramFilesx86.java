package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.scripts.client.OAInvalidInstallerLocation;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OAInvalidInstallerLocation - C:\Program Files (x86)\
 *
 * @author micwang
 */
public class OASetInstallerLocationProgramFilesx86 extends OAInvalidInstallerLocation {

    @TestRail(caseId = 10076)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testSetInstallerLocationProgramFiles(ITestContext context) throws Exception {
        testInvalidInstallerLocation(context, "C:\\Program Files (x86)\\");
    }
}
