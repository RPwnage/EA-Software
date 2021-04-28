package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.gamedownloader.OABasicPauseResumeCancel;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parameterized test case for OABasicPauseResumeCancel with the DiP Entitlement
 *
 * @author glivingstone
 */
public class OABasicPauseResumeCancelDiP extends OABasicPauseResumeCancel {

    @Test(groups = {"gamedownloader", "long_smoke", "client_only"})
    public void testPauseResumeCancelDiP(ITestContext context) throws Exception {
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        testBasicPauseResumeCancel(context, entitlement, this.getClass().getName());
    }
}
