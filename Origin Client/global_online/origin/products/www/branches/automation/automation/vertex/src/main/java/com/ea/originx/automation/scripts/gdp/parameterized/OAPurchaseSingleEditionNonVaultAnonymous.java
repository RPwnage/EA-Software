package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseSingleEditionNonVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a single edition non vault entitlement (browser)
 *
 * @author cdeaconu
 */
public class OAPurchaseSingleEditionNonVaultAnonymous extends OAPurchaseSingleEditionNonVault{
    
    @TestRail(caseId = 3001997)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPurchaseSingleEditionNonVaultAnonymous(ITestContext context) throws Exception{
        testPurchaseSingleEditionNonVault(context, OAPurchaseSingleEditionNonVault.TEST_TYPE.ANONYMOUS_USER);
    }
}
