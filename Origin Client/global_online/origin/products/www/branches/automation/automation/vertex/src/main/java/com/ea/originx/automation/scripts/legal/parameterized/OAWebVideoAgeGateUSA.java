package com.ea.originx.automation.scripts.legal.parameterized;

import com.ea.originx.automation.scripts.legal.OAWebVideoAgeGate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that anonymous users in the United States are unable to view age gated videos
 *
 * @author cvanichsarn
 */
public class OAWebVideoAgeGateUSA extends OAWebVideoAgeGate {

    @TestRail(caseId = 1016686)
    @Test(groups = {"legal", "browser_only", "release_smoke"})
    public void testWebVideoAgeGateUSA(ITestContext context) throws Exception {
        testWebVideoAgeGate(context, CountryInfo.CountryEnum.USA);
    }
}
