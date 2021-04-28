package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseMultipleEditionNonVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a multiple edition Non-Vault game for SignedIn user not subscribed to Origin Access
 *
 * @author tdhillon
 */
public class OAPurchaseMultipleEditionNonVaultNonSubscriber extends OAPurchaseMultipleEditionNonVault{

    @TestRail(caseId = 3068475)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseMultipleEditionNonVaultNonSubscriber(ITestContext context) throws Exception{
        testPurchaseMultipleEditionNonVault(context, OAPurchaseMultipleEditionNonVault.TEST_TYPE.NON_SUBSCRIBER);
    }
}
