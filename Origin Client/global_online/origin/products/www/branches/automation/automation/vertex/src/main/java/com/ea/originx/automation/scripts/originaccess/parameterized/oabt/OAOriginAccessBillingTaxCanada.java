package com.ea.originx.automation.scripts.originaccess.parameterized.oabt;

import com.ea.originx.automation.scripts.originaccess.OAOriginAccessBillingTax;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.LanguageInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verify tax information when purchasing Origin Access in Canada region
 *
 * @author ivlim
 */
public class OAOriginAccessBillingTaxCanada extends OAOriginAccessBillingTax {

    @TestRail(caseId = 11014)
    @Test(groups = {"originaccess", "full_regression"})
    public void testOriginAccessBillingTaxCanada(ITestContext context) throws Exception {
        testOriginAccessBillingTax(context, CountryInfo.CountryEnum.CANADA, LanguageInfo.LanguageEnum.ENGLISH);
    }
}
