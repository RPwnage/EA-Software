package com.ea.originx.automation.scripts.wishlist;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOfflineMode;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileOfflinePage;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;


/**
 * Tests the Shortcut Key Ctrl-Stift-W navigates to the Wishlist
 *
 * @author nvarthakavi
 * @author ivlim
 */
public class OAShortcutWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 11718)
    @Test(groups = {"wishlist", "client_only", "full_regression"})
    public void testShortcutWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount wishlistAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_CANADA);
        final String wishlistAccountName = wishlistAccount.getUsername();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String entitlementName = entitlement.getName();

        logFlowPoint(String.format("Launch Origin and login as user '%s'", wishlistAccountName)); // 1
        logFlowPoint(String.format("Add '%s' to wishlist from their corresponding GDP Page", entitlementName)); // 2
        logFlowPoint(String.format("From the top bar, Click on View > Wishlist and verify it navigates to the Wishlist Page", entitlementName)); // 3
        logFlowPoint("Navigate to Home Page and press Ctrl-Shift-W short-cut and verify it navigates to the Wishlist Page"); // 4
        logFlowPoint("Navigate to Home Page and change to offline mode, click on View > Wishlist and verify it navigates to the Offline Mode Page"); // 5
        logFlowPoint("While offline, navigate to Home Page, press Ctrl-Shift-W shortcut and verify it navigates to the Wishlist Page Page"); // 6

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, wishlistAccount), true);

        //2
        logPassFail(MacroWishlist.addToWishlist(driver, entitlement), true);

        //3
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectWishlist();
        logPassFail(new ProfileHeader(driver).verifyWishlistActive() && new ProfileWishlistTab(driver).verifyTilesExist(entitlement.getOfferId()), true);

        //4
        new NavigationSidebar(driver).gotoDiscoverPage();
        MacroWishlist.navigateToWishlistUsingShortcut(driver);
        logPassFail(new ProfileHeader(driver).verifyWishlistActive() && new ProfileWishlistTab(driver).verifyTilesExist(entitlement.getOfferId()), true);

        //5
        MacroOfflineMode.goOffline(driver);
        new NavigationSidebar(driver).gotoDiscoverPage();
        mainMenu.selectWishlist();
        logPassFail(new ProfileOfflinePage(driver).verifyOnOfflinePage(), true);

        //6
        new NavigationSidebar(driver).gotoDiscoverPage();
        MacroWishlist.navigateToWishlistUsingShortcut(driver);
        logPassFail(new ProfileOfflinePage(driver).verifyOnOfflinePage(), true);

        softAssertAll();
    }
}
