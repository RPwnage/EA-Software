package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPreorderMultipleEditionGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test preorder multiple edition game with an anonymous user
 *
 * @author mirani
 */
public class OAPreorderMultipleEditionGameAnonymous extends OAPreorderMultipleEditionGame{
    @TestRail(caseId = 3086930)
    @Test(groups = {"gdp", "full_regression", "browser_only"})
    public void testPreorderMultipleEditionGameAnonymous(ITestContext context) throws Exception{
        testPreorderMultipleEditionGame(context,  TEST_TYPE.ANONYMOUS_USER);
    }
}
