package com.ea.originx.automation.scripts.client.parameterized;

import com.ea.originx.automation.scripts.client.OAInvalidInstallerLocation;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test script for OAInvalidInstallerLocation - C:\Windows\
 *
 * @author micwang
 */
public class OASetInstallerLocationWindows extends OAInvalidInstallerLocation {

    @TestRail(caseId = 10077)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testSetInstallerLocationProgramFiles(ITestContext context) throws Exception {
        testInvalidInstallerLocation(context, "C:\\Windows\\");
    }
}
