package com.ea.originx.automation.scripts.recommendation.parameterized;

import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.originx.automation.scripts.recommendation.OARecFriendPrivacy;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test for 'Recommended Friends' with privacy setting of 'Friends'
 *
 * @author palui
 */
public class OARecFriendPrivacyFriend extends OARecFriendPrivacy {

    @TestRail(caseId = 164163)
    @Test(groups = {"recommendation", "full_regression"})
    public void testOARecFriendPrivacyFriend(ITestContext context) throws Exception {
        testRecFriendPrivacy(context, PrivacySetting.FRIENDS);
    }
}
