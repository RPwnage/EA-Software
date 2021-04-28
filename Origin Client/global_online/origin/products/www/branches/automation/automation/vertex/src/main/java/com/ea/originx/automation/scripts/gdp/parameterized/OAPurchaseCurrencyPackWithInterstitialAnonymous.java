package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseCurrencyPackWithInterstitial;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a currency pack from gdp with interstitial as an anonymous user
 *
 * @author mirani
 */
public class OAPurchaseCurrencyPackWithInterstitialAnonymous extends OAPurchaseCurrencyPackWithInterstitial {

    @TestRail(caseId = 3101358)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPurchaseCurrencyPackWithInterstitialAnonymous(ITestContext context) throws Exception{
        testPurchaseCurrencyPackWithInterstitial(context, TEST_TYPE.ANONYMOUS_USER);
    }
}
