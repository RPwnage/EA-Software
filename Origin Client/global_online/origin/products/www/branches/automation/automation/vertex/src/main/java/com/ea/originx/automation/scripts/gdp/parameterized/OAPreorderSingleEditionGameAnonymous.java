package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPreorderSingleEditionGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test pre-ordering a Single edition game for Anonymous User
 *
 * @author tdhillon
 */
public class OAPreorderSingleEditionGameAnonymous extends OAPreorderSingleEditionGame {

    @TestRail(caseId = 3001998)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPreorderSingleEditionGameAnonymous(ITestContext context) throws Exception{
        testPreorderSingleEditionGame(context,  TEST_TYPE.ANONYMOUS_USER);
    }
}
