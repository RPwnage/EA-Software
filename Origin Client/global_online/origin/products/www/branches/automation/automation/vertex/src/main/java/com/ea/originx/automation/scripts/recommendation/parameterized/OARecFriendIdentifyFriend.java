package com.ea.originx.automation.scripts.recommendation.parameterized;

import com.ea.originx.automation.scripts.recommendation.OARecFriendIdentify;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test identifying 'Recommended Friends' among 5 users with privacy settings of
 * 'Everyone' for users 1 and 2, 'Friends' for friend users 3 and 4, and
 * 'Friends Of Friends' for friend user 5
 *
 * @author palui
 */
public class OARecFriendIdentifyFriend extends OARecFriendIdentify {

    @TestRail(caseId = 158312)
    @Test(groups = {"recommendation", "full_regression"})
    public void testRecFriendIdentifyFriend(ITestContext context) throws Exception {
        testRecFriendIdentify(context, TEST_TYPE.FRIEND);
    }
}
