package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.gamedownloader.OABasicPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OABasicPauseResumeCancel with the Non-Dip
 * Entitlement
 *
 * @author glivingstone
 */
public class OABasicPauseResumeCancelZip extends OABasicPauseResumeCancel {

    @Test(groups = {"gamedownloader", "long_smoke", "client_only"})
    public void testPauseResumeCancelDiP(ITestContext context) throws Exception {
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_LARGE);
        testBasicPauseResumeCancel(context, entitlement, this.getClass().getName());
    }
}
