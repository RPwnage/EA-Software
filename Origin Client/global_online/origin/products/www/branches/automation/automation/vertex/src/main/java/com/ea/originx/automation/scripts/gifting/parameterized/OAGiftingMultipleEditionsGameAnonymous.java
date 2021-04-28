package com.ea.originx.automation.scripts.gifting.parameterized;

import com.ea.originx.automation.scripts.gifting.OAGiftingMultipleEditionsGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test gifting a multiple editions game for an anonymous user (browser only)
 *
 * @author cdeaconu
 */
public class OAGiftingMultipleEditionsGameAnonymous extends OAGiftingMultipleEditionsGame{
    
    @TestRail(caseId = 3129098)
    @Test(groups = {"gdp", "gifting", "full_regression", "browser_only"})
    public void testGiftingMultipleEditionsGameAnonymous(ITestContext context) throws Exception{
        testGiftingMultipleEditionsGame(context, OAGiftingMultipleEditionsGame.TEST_TYPE.ANONYMOUS_USER);
    }
}