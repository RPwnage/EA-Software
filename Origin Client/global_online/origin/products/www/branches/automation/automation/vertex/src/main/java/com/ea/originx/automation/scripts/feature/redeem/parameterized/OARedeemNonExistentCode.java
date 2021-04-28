package com.ea.originx.automation.scripts.feature.redeem.parameterized;

import com.ea.originx.automation.scripts.feature.redeem.OARedeemInvalidCode;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that redeeming a non-existent code does not work
 *
 * @author jdickens
 */
public class OARedeemNonExistentCode extends OARedeemInvalidCode {

    @TestRail(caseId = 10002)
    @Test(groups = {"redeem", "redeem_smoke", "client_only", "allfeaturesmoke", "full_regression"})
    public void testRedeemNonExistentCode(ITestContext context) throws Exception {
        testRedeemInvalidCode(context, TEST_TYPE.INVALID);
    }
}