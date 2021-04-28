package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseSingleEditionNonVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a single edition non vault entitlement with a non-subscriber
 * account
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionNonVaultNonSubscriber extends OAPurchaseSingleEditionNonVault{
    
    @TestRail(caseId = 3001999)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseSingleEditionNonVaultNonSubscriber(ITestContext context) throws Exception{
        testPurchaseSingleEditionNonVault(context, OAPurchaseSingleEditionNonVault.TEST_TYPE.NON_SUBSCRIBER);
    }
}
