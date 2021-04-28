package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAPauseResumeCancel;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OAPauseResumeCancel with parameters:
 * Pause: false
 * Resume: false
 * EntitlementType: ENTITLEMENT_TYPE.DIP_WITH_DLC
 *
 * @author glivingstone
 */
public class OADownloadCancelDiP extends OAPauseResumeCancel {

    @TestRail(caseId = 9849)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadCancelDiP(ITestContext context) throws Exception {
        testPauseResumeCancel(context, false, false, ENTITLEMENT_TYPE.DIP_WITH_DLC, this.getClass().getName());
    }

}
