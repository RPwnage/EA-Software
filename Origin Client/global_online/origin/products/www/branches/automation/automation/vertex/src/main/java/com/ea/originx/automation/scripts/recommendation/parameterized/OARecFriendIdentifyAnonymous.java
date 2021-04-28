package com.ea.originx.automation.scripts.recommendation.parameterized;

import com.ea.originx.automation.scripts.recommendation.OARecFriendIdentify;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test identifying 'Recommended Friends' among 5 users with privacy settings of
 * 'Everyone' for users 1-2 and friend users 3-4, and 'No One' for friend user
 * 5.
 *
 * @author palui
 */
public class OARecFriendIdentifyAnonymous extends OARecFriendIdentify {

    @TestRail(caseId = 158317)
    @Test(groups = {"recommendation", "full_regression"})
    public void testRecFriendsIdentifyAnonymous(ITestContext context) throws Exception {
        testRecFriendIdentify(context, TEST_TYPE.ANONYMOUS);
    }
}
