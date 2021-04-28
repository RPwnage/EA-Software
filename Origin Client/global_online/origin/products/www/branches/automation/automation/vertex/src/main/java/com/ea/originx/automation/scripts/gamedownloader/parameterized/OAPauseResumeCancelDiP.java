package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OAPauseResumeCancel with parameters:
 * Pause: true
 * Resume: true
 * EntitlementType: ENTITLEMENT_TYPE.DIP_WITHOUT_DLC
 *
 * @author glivingstone
 */
public class OAPauseResumeCancelDiP extends OAPauseResumeCancel {

    @Test(groups = {"gamedownloader", "client_only", "full_regression", "gamedownloader_smoke"})
    public void testPauseResumeCancelDiP(ITestContext context) throws Exception {
        testPauseResumeCancel(context, true, true, ENTITLEMENT_TYPE.DIP_WITHOUT_DLC, this.getClass().getName());
    }

}
