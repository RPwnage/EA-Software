package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Add an entitlement to the 'Wishlist' of a throwaway account and check if it is visible
 * in the 'Wishlist' tab
 *
 * @author jdickens
 */
public class OABasicWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 3121920)
    @Test(groups = {"wishlist", "long_smoke"})
    public void testBasicWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        String entitlementName = entitlementInfo.getName();
        UserAccount user = AccountManager.getInstance().createNewThrowAwayAccount();
        String userAccountName = user.getUsername();

        logFlowPoint("Log into the Origin client with a throwaway account"); // 1
        logFlowPoint("Add an entitlement to the user's 'Wishlist'"); // 2

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Logged into the Origin client with account " + userAccountName);
        } else {
            logFailExit("Failed to log into the Origin client with account " + userAccountName);
        }

        // 2
        if (MacroWishlist.addToWishlist(driver, entitlementInfo)) {
            logPass("Successfully verified that " + entitlementName + " has been added to the 'Wishlist'");
        } else {
            logFailExit("Failed to verify that " + entitlementName + " has been added to the 'Wishlist'");
        }

        softAssertAll();
    }
}