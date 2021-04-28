package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseMultipleEditionNonVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a multiple edition Non-Vault game for Anonymous User
 *
 * @author tdhillon
 */
public class OAPurchaseMultipleEditionNonVaultAnonymous extends OAPurchaseMultipleEditionNonVault {

    @TestRail(caseId = 3068474)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPurchaseMultipleEditionNonVaultAnonymous(ITestContext context) throws Exception{
        testPurchaseMultipleEditionNonVault(context, OAPurchaseMultipleEditionNonVault.TEST_TYPE.ANONYMOUS_USER);
    }
}
