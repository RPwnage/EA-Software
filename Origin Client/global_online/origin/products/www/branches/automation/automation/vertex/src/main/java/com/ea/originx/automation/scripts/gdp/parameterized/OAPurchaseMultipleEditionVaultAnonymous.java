package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseMultipleEditionVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a multiple edition vault entitlement (browser)
 *
 * @author mdobre
 */
public class OAPurchaseMultipleEditionVaultAnonymous extends OAPurchaseMultipleEditionVault {

    @TestRail(caseId = 3068471)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPurchaseMultipleEditionVaultAnonymous(ITestContext context) throws Exception {
        testPurchaseMultipleEditionVault(context, OAPurchaseMultipleEditionVault.TEST_TYPE.ANONYMOUS_USER);
    }
}