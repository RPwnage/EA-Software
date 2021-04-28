package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAAddDemoTrial;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding Trial Entitlement on an account with 'Origin Access' for a
 * subscriber
 *
 * @author cbalea
 */
public class OAAddDemoTrialSubscriber extends OAAddDemoTrial {

    @TestRail(caseId = 3123729)
    @Test(groups = {"gdp", "full_regression"})
    public void testAddDemoTrialSubscriber(ITestContext context) throws Exception {
        testAddDemoTrial(context, OAAddDemoTrial.TEST_TYPE.SUBSCRIBER);
    }
}
