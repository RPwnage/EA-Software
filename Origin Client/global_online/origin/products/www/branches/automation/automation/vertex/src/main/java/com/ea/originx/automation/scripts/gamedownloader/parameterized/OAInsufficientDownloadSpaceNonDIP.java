package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAInsufficientDownloadSpace;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the insufficient download space for a Non-DIP entitlement
 *
 * @author cvanichsarn
 */
public class OAInsufficientDownloadSpaceNonDIP extends OAInsufficientDownloadSpace {
    
    @TestRail(caseId = 9836)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testInsufficientDownloadSpaceNonDIP(ITestContext context) throws Exception {
        testInSufficientDownloadSpace(context, false);
    } 
}
