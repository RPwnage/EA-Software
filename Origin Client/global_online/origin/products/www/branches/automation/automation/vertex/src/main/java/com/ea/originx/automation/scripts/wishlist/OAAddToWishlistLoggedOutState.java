package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.SystemMessage;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.OriginClientConstants;
import java.util.ArrayList;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Testing adding wishlist from GDP page when the user has not logged in
 *
 * @author RCHOI
 */
public class OAAddToWishlistLoggedOutState extends EAXVxTestTemplate {

    @TestRail(caseId = 11713)
    @Test(groups = {"wishlist", "full_regression", "browser_only"})
    public void testAddToWishlistLoggedOutState(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final String userAccountName = userAccount.getUsername();
        final String userAccountEmailAddress = userAccount.getEmail();
        final String userAccountPassword = userAccount.getPassword();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Verify login successful by using new account: " + userAccountName + " then logout after logging in");  // 1
        logFlowPoint("Verify Login window appears after clicking 'Add to wishlist' button on pdp page when no user has logged in.");  // 2
        logFlowPoint("Login and verify an animated heart and the message '" + entitlementName + " is on your wishlist' appears.");  // 3
        logFlowPoint("Verify it navigates to wishlist tab by clicking 'view wishlist' link on pdp page");  // 4
        logFlowPoint("Verify the entitlement " + entitlementName + " is on the wishlist");  // 5

        //1
        final WebDriver browserDriver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(browserDriver, userAccount), true);
        new MiniProfile(browserDriver).selectSignOut();

        //2
        // closing sitestripe as this may cause 'not finding dropdown button element correctly' issue
        SystemMessage siteStripe = new SystemMessage(browserDriver);
        if (siteStripe.isSiteStripeVisible()) {
            siteStripe.closeSiteStripe();
        }
        
        MacroWishlist.addToWishlist(browserDriver, entitlement);
        ArrayList<String> tabs = new ArrayList<>(browserDriver.getWindowHandles());
        // switch to login window
        browserDriver.switchTo().window(tabs.get(1));
        final LoginPage loginPage = new LoginPage(browserDriver);
        loginPage.waitForPageToLoad();
        logPassFail(loginPage.verifyTitleIs("Sign in with your EA Account"), true);

        //3
        loginPage.enterInformation(userAccountEmailAddress, userAccountPassword);
        // switch back to parent window
        browserDriver.switchTo().window(tabs.get(0));
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(browserDriver);
        boolean isOnWishlistPDP = offerSelectionPage.verifyOnWishlist();
        logPassFail(isOnWishlistPDP, true);

        //4
        offerSelectionPage.clickViewWishlistLink();
        logPassFail(new ProfileHeader(browserDriver).verifyWishlistActive(), true);

        //5
        boolean isEntitlementOnWishlist = new ProfileWishlistTab(browserDriver).verifyTilesExist(entitlement.getOfferId());
        logPassFail(isEntitlementOnWishlist, true);
        
        softAssertAll();
    }
}