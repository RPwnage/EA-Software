package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OAPauseResumeCancel with parameters:
 * Pause: false
 * Resume: false
 * EntitlementType: ENTITLEMENT_TYPE.NON_DIP
 *
 * @author glivingstone
 */
public class OADownloadCancelZip extends OAPauseResumeCancel {

    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testDownloadCancelZip(ITestContext context) throws Exception {
        testPauseResumeCancel(context, false, false, ENTITLEMENT_TYPE.NON_DIP, this.getClass().getName());
    }

}
