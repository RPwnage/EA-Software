package com.ea.originx.automation.scripts.feature.wishlist.parameterized;

import com.ea.originx.automation.scripts.feature.wishlist.OAWishlistPrivacy;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests wishlist privacy setting Everyone
 *
 * @author cvanichsarn
 */
public class OAWishlistPrivacyEveryone extends OAWishlistPrivacy {

    @TestRail(caseId = 158306)
    @Test(groups = {"wishlist_smoke", "wishlist", "full_regression", "allfeaturesmoke","release_smoke"})
    public void testWishlistPrivacyEveryone(ITestContext context) throws Exception {
        testWishlistPrivacy(context, PrivacySetting.EVERYONE);
    }
}
