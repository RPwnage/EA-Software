package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseSingleEditionNonVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a single edition non vault entitlement with an Origin Access
 * subscriber account
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionNonVaultSubscriber extends OAPurchaseSingleEditionNonVault{
    
    @TestRail(caseId = 3087358)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseSingleEditionNonVaultSubscriber(ITestContext context) throws Exception{
        testPurchaseSingleEditionNonVault(context, OAPurchaseSingleEditionNonVault.TEST_TYPE.SUBSCRIBER);
    }
}
