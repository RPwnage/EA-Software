package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OAPauseResumeCancel with parameters:
 * Pause: true
 * Resume: false
 * EntitlementType: ENTITLEMENT_TYPE.NON_DIP
 *
 * @author glivingstone
 */
public class OAPauseCancelZip extends OAPauseResumeCancel {

    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPauseCancelZip(ITestContext context) throws Exception {
        testPauseResumeCancel(context, true, false, ENTITLEMENT_TYPE.NON_DIP, this.getClass().getName());
    }

}
