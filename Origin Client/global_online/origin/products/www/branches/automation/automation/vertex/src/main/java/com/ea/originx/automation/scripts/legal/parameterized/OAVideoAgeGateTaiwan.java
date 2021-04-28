package com.ea.originx.automation.scripts.legal.parameterized;

import com.ea.originx.automation.scripts.legal.OAVideoAgeGate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that underage users in Taiwan cannot view above age videos
 *
 * @author cvanichsarn
 */
public class OAVideoAgeGateTaiwan extends OAVideoAgeGate {

    @TestRail(caseId = 1433868)
    @Test(groups = {"legal", "client_only","release_smoke", "int_only"})
    public void testVideoAgeGateTaiwan(ITestContext context) throws Exception {
        testVideoAgeGate(context, CountryInfo.CountryEnum.TAIWAN);
    }
}
