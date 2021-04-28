package com.ea.originx.automation.scripts.feature.redeem.parameterized;

import com.ea.originx.automation.scripts.feature.redeem.OARedeemInvalidCode;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that redeeming an already used code does not work
 *
 * @author jdickens
 */
public class OARedeemUsedCode extends OARedeemInvalidCode {

    @TestRail(caseId = 10001)
    @Test(groups = {"redeem", "redeem_smoke", "client_only", "allfeaturesmoke", "full_regression"})
    public void testRedeemUsedCode(ITestContext context) throws Exception {
        testRedeemInvalidCode(context, TEST_TYPE.ALREADY_USED);
    }
}
