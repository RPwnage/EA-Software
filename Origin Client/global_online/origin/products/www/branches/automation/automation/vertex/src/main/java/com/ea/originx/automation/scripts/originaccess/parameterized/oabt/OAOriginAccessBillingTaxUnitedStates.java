package com.ea.originx.automation.scripts.originaccess.parameterized.oabt;

import com.ea.originx.automation.scripts.originaccess.OAOriginAccessBillingTax;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.LanguageInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verify tax information when purchasing Origin Access in UnitedStates region
 *
 * @author ivlim
 */
public class OAOriginAccessBillingTaxUnitedStates extends OAOriginAccessBillingTax {

    @TestRail(caseId = 11014)
    @Test(groups = {"originaccess", "full_regression"})
    public void testOriginAccessBillingTaxUnitedStates(ITestContext context) throws Exception {
        testOriginAccessBillingTax(context, CountryInfo.CountryEnum.USA, LanguageInfo.LanguageEnum.ENGLISH);
    }
}
