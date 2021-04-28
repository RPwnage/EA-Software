package com.ea.originx.automation.scripts.social.parameterized;

import com.ea.originx.automation.scripts.social.OAProfilePresence;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parametrized test to check the appearance of a user's profile as the presence status changes. Verifies changes
 * in both the user's client and the client of a friend.
 *
 * @author jdickens
 */
public class OAProfileFriendsPresence extends OAProfilePresence {

    @TestRail(caseId = 2910986)
    @Test(groups = {"social", "client_only"})
    public void testProfileFriendsPresence(ITestContext context) throws Exception {
        testProfilePresence(context, TEST_TYPE.NON_RELEASE);
    }
}