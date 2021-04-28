package com.ea.originx.automation.scripts.gifting.parameterized;

import com.ea.originx.automation.scripts.gifting.OAGiftingMultipleEditionsGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test gifting a multiple editions game for a signed in user
 *
 * @author cdeaconu
 */
public class OAGiftingMultipleEditionsGameSignedUser extends OAGiftingMultipleEditionsGame{
    
    @TestRail(caseId = 3129099)
    @Test(groups = {"gdp", "gifting", "full_regression"})
    public void testGiftingMultipleEditionsGameSignedUser(ITestContext context) throws Exception{
        testGiftingMultipleEditionsGame(context, OAGiftingMultipleEditionsGame.TEST_TYPE.SIGNED_IN);
    }
}