package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OAPauseResumeCancel with parameters:
 * Pause: true
 * Resume: true
 * EntitlementType: ENTITLEMENT_TYPE.NON_DIP
 *
 * @author glivingstone
 */
public class OAPauseResumeCancelZip extends OAPauseResumeCancel {

    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPauseResumeCancelZip(ITestContext context) throws Exception {
        testPauseResumeCancel(context, true, true, ENTITLEMENT_TYPE.NON_DIP, this.getClass().getName());
    }

}
