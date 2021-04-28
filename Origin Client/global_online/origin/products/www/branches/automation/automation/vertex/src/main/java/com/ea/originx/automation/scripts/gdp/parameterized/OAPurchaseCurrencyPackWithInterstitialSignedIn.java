package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPurchaseCurrencyPackWithInterstitial;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchase a currency pack from gdp with interstitial as a signed in user
 *
 * @author mirani
 */
public class OAPurchaseCurrencyPackWithInterstitialSignedIn extends OAPurchaseCurrencyPackWithInterstitial {

    @TestRail(caseId = 3101359)
    @Test(groups = {"gdp", "full_regression"})
    public void testPurchaseCurrencyPackWithInterstitialSignedIn(ITestContext context) throws Exception{
        testPurchaseCurrencyPackWithInterstitial(context, TEST_TYPE.SIGNED_IN);
    }
}
