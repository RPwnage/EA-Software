package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseSingleEditionVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a single edition vault entitlement with a non-subscriber
 * account (browser)
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionVaultAnonymous extends OAPurchaseSingleEditionVault{
    
    @TestRail(caseId = 3064200)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPurchaseSingleEditionVaultAnonymous(ITestContext context) throws Exception {
        testPurchaseSingleEditionVault(context, OAPurchaseSingleEditionVault.TEST_TYPE.ANONYMOUS_USER);
    }
}
