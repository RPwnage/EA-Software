package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAGDPHeaderRating;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the game rating in 'GDP' is country specific
 *
 * @author cbalea
 */
public class OAGDPHeaderRatingGermany extends OAGDPHeaderRating {

    @TestRail(caseId = 3001994)
    @Test(groups = {"gdp", "full_regression"})
    public void testGDPHeaderRatingGermany(ITestContext context) throws Exception {
        testGDPHeaderRating(context, CountryInfo.CountryEnum.GERMANY);
    }

}
