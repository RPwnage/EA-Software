package com.ea.originx.automation.scripts.recommendation.parameterized;

import com.ea.originx.automation.scripts.recommendation.OARecFriendIdentify;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test identifying 'Recommended Friends' among 5 users with privacy setting of
 * 'Everyone'
 *
 * @author palui
 */
public class OARecFriendIdentifyEveryone extends OARecFriendIdentify {

    @TestRail(caseId = 158311)
    @Test(groups = {"recommendation", "full_regression"})
    public void testRecFriendsIdentifyEveryone(ITestContext context) throws Exception {
        testRecFriendIdentify(context, TEST_TYPE.EVERYONE);
    }
}
