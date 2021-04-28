package com.ea.originx.automation.scripts.social.parameterized;

import com.ea.originx.automation.scripts.social.OAProfilePresence;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Parametrized test to check the appearance of a user's profile as the presence status changes. Verifies changes
 * in both the user's client and the client of a friend, and additionally checks that a stranger's presence is not
 * visible
 *
 * @author glivingstone
 */
public class OAProfilePresenceRelease extends OAProfilePresence {

    @TestRail(caseId = 1016709)
    @Test(groups = {"social", "release_smoke", "client_only"})
    public void testProfilePresenceRelease(ITestContext context) throws Exception {
        testProfilePresence(context, TEST_TYPE.RELEASE);
    }
}