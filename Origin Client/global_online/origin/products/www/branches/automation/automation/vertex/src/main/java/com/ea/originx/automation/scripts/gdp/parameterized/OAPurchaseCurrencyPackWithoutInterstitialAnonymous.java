package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseCurrencyPackWithoutInterstitial;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a currency pack without interstitial as an anonymous user
 *
 * @author svaghayenegar
 */
public class OAPurchaseCurrencyPackWithoutInterstitialAnonymous extends OAPurchaseCurrencyPackWithoutInterstitial {

    @TestRail(caseId = 3096146)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPurchaseCurrencyPackWithoutInterstitialAnonymous(ITestContext context) throws Exception {
        testPurchaseCurrencyPackWithoutInterstitial(context, TEST_TYPE.ANONYMOUS_USER);
    }
}
