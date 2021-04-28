package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseSingleEditionVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a single edition vault entitlement with a non-subscriber
 * account
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionVaultNonSubscriber extends OAPurchaseSingleEditionVault{
    
    @TestRail(caseId = 3002228)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseSingleEditionVaultNonSubscriber(ITestContext context) throws Exception {
        testPurchaseSingleEditionVault(context, OAPurchaseSingleEditionVault.TEST_TYPE.NON_SUBSCRIBER);
    }
}
