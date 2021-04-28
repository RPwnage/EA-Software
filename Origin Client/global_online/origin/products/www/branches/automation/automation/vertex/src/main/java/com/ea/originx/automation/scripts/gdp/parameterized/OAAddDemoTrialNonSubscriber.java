package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAAddDemoTrial;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding Trial Entitlement on an account without 'Origin Access' for a
 * non-subscriber
 *
 * @author cbalea
 */
public class OAAddDemoTrialNonSubscriber extends OAAddDemoTrial {

    @TestRail(caseId = 3065110)
    @Test(groups = {"gdp", "full_regression"})
    public void testAddDemoTrialNonSubscriber(ITestContext context) throws Exception {
        testAddDemoTrial(context, OAAddDemoTrial.TEST_TYPE.NON_SUBSCRIBER);
    }
}
