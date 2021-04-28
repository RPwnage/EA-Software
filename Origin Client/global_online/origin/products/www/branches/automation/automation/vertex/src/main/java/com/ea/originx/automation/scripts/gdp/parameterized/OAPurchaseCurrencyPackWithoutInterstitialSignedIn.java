package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseCurrencyPackWithoutInterstitial;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a currency pack without interstitial as a signed in user
 *
 * @author svaghayenegar
 */
public class OAPurchaseCurrencyPackWithoutInterstitialSignedIn extends OAPurchaseCurrencyPackWithoutInterstitial {

    @TestRail(caseId = 3096147)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseCurrencyPackWithoutInterstitialAnonymous(ITestContext context) throws Exception {
        testPurchaseCurrencyPackWithoutInterstitial(context, TEST_TYPE.SIGNED_IN_USER);
    }
}
