package com.ea.originx.automation.scripts.gdp.parameterized;

import com.ea.originx.automation.scripts.gdp.OAPreorderMultipleEditionGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test preorder multiple edition game with a signed in user
 *
 * @author mirani
 */
public class OAPreorderMultipleEditionGameSignedIn extends OAPreorderMultipleEditionGame{
    @TestRail(caseId = 3086931)
    @Test(groups = {"gdp", "full_regression"})
    public void testPreorderMultipleEditionGameSignedIn(ITestContext context) throws Exception{
        testPreorderMultipleEditionGame(context,  TEST_TYPE.SIGNED_IN);
    }
}
