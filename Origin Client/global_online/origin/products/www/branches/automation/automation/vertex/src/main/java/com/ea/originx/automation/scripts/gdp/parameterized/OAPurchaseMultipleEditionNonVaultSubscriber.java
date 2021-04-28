package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseMultipleEditionNonVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a multiple edition Non-Vault game for Origin Access Subscriber
 *
 * @author tdhillon
 */
public class OAPurchaseMultipleEditionNonVaultSubscriber extends OAPurchaseMultipleEditionNonVault {

    @TestRail(caseId = 3087362)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseMultipleEditionNonVaultSubscriber(ITestContext context) throws Exception{
        testPurchaseMultipleEditionNonVault(context, OAPurchaseMultipleEditionNonVault.TEST_TYPE.SUBSCRIBER);
    }
}
