package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPreorderSingleEditionGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test pre-ordering a Single edition game for SignedIn User
 *
 * @author tdhillon
 */
public class OAPreorderSingleEditionGameSignedIn extends OAPreorderSingleEditionGame{

    @TestRail(caseId = 3063709)
    @Test(groups = {"gdp", "full_regression"})
    public void testPreorderSingleEditionGameSignedIn(ITestContext context) throws Exception{
        testPreorderSingleEditionGame(context,  OAPreorderSingleEditionGame.TEST_TYPE.SIGNED_IN);
    }
}
