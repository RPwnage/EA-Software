package com.ea.originx.automation.scripts.legal.parameterized;

import com.ea.originx.automation.scripts.legal.OAVideoAgeGate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that underage users in the United States cannot view above age videos
 *
 * @author cvanichsarn
 */
public class OAVideoAgeGateUSA extends OAVideoAgeGate {

    @TestRail(caseId = 1433869)
    @Test(groups = {"legal", "client_only","release_smoke"})
    public void testVideoAgeGateUSA(ITestContext context) throws Exception {
        testVideoAgeGate(context, CountryInfo.CountryEnum.USA);
    }
}
