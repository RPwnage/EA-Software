package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAOfferSelectionPageRating;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.CountryInfo;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the game rating in 'OSP' for games with multiple editions is country specific
 *
 * @author alcervantes
 */
public class OAOfferSelectionPageRatingSouthKorea extends OAOfferSelectionPageRating {

    @TestRail(caseId = 3001994)
    @Test(groups = {"gdp", "full_regression"})
    public void testOfferSelectionPageRatingSouthKorea(ITestContext context) throws Exception {
        testOfferSelectionPageRating(context, CountryInfo.CountryEnum.SOUTH_KOREA);
    }
}
