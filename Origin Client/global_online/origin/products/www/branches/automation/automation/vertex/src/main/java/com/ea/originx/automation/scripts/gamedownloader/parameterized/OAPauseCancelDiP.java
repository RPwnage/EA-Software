package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OAPauseResumeCancel with parameters:
 * Pause: true
 * Resume: false
 * EntitlementType: ENTITLEMENT_TYPE.DIP_WITHOUT_DLC
 *
 * @author glivingstone
 */
public class OAPauseCancelDiP extends OAPauseResumeCancel {

    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPauseCancelDiP(ITestContext context) throws Exception {
        testPauseResumeCancel(context, true, false, ENTITLEMENT_TYPE.DIP_WITHOUT_DLC, this.getClass().getName());
    }

}
