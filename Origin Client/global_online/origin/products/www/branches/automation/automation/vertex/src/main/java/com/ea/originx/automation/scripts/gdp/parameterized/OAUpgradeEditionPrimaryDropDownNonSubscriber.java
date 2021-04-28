package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAUpgradeEditionPrimaryDropDown;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test clicking 'Upgrade my edition' option in the drop-down menu redirects the
 * user to the 'Offer Selection Page Interstitial' page
 *
 * @author cdeaconu
 */
public class OAUpgradeEditionPrimaryDropDownNonSubscriber extends OAUpgradeEditionPrimaryDropDown{
    
    @TestRail(caseId = 3064029)
    @Test(groups = {"gdp", "full_regression"})
    public void testUpgradeEditionPrimaryDropDownNonSubscriber(ITestContext context) throws Exception{
        testUpgradeEditionPrimaryDropDown(context, OAUpgradeEditionPrimaryDropDown.TEST_TYPE.NON_SUBSCRIBER);
    }
}