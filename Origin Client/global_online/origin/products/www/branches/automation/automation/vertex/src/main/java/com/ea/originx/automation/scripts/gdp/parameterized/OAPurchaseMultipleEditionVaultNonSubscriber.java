package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseMultipleEditionVault;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a multiple edition vault entitlement with a non-subscriber
 * account
 *
 * @author mdobre
 */
public class OAPurchaseMultipleEditionVaultNonSubscriber extends OAPurchaseMultipleEditionVault {

    @TestRail(caseId = 3068472)
    @Test(groups = {"gdp", "full_regression", "release_smoke"})
    public void testPurchaseMultipleEditionVaultNonSubscriber(ITestContext context) throws Exception {
        testPurchaseMultipleEditionVault(context, OAPurchaseMultipleEditionVault.TEST_TYPE.NON_SUBSCRIBER);
    }
}