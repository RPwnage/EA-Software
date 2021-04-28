package com.ea.originx.automation.scripts.legal.parameterized;

import com.ea.originx.automation.scripts.legal.OAWebVideoAgeGate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that anonymous users in Taiwan are unable to view age gated videos
 *
 * @author cvanichsarn
 */
public class OAWebVideoAgeGateTaiwan extends OAWebVideoAgeGate {

    @TestRail(caseId = 1433867)
    @Test(groups = {"legal", "browser_only", "release_smoke", "int_only"})
    public void testWebVideoAgeGateTaiwan(ITestContext context) throws Exception {
        testWebVideoAgeGate(context, CountryInfo.CountryEnum.TAIWAN);
    }
}
