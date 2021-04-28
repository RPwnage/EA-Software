package com.ea.originx.automation.scripts.feature.wishlist.parameterized;

import com.ea.originx.automation.scripts.feature.wishlist.OAWishlistPrivacy;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests wishlist privacy setting No One
 *
 * @author cvanichsarn
 */
public class OAWishlistPrivacyNoOne extends OAWishlistPrivacy {

    @TestRail(caseId = 158309)
    @Test(groups = {"wishlist_smoke", "wishlist", "full_regression", "allfeaturesmoke","release_smoke"})
    public void testWishlistPrivacyNoOne(ITestContext context) throws Exception {
        testWishlistPrivacy(context, PrivacySetting.NO_ONE);
    }
}
