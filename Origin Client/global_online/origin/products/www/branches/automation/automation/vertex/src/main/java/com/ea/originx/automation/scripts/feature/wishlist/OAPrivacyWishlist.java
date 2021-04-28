package com.ea.originx.automation.scripts.feature.wishlist;

import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroWishlist;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebDriverException;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the wishlist privacy settings
 *
 * @author jmittertreiner
 */
public class OAPrivacyWishlist extends EAXVxTestTemplate {

    @TestRail(caseId = 3120666)
    @Test(groups = {"wishlist_smoke", "wishlist", "allfeaturesmoke"})
    public void testPrivacyWishlist(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final WebDriver driver = startClientObject(context, client);
        final UserAccount user = AccountManager.getInstance().createNewThrowAwayAccount();
        final UserAccount friend = AccountManager.getInstance().createNewThrowAwayAccount();
        final String userAccount = user.getUsername();
        final String friendAccount = friend.getUsername();
        final Battlefield4 entitlement = new Battlefield4();

        logFlowPoint("Login to Origin with " + userAccount); // 1
        logFlowPoint("Add any entitlement to the " + userAccount + "'s wishlist"); // 2
        logFlowPoint("Navigate to the wishlist"); // 3
        logFlowPoint("Verify: A link appears for others to view the wishlist"); // 4
        logFlowPoint("Open a browser and log into " + friendAccount + " on it"); // 5
        logFlowPoint("Navigate to the wishlist link on " + userAccount + " 's page"); // 6
        logFlowPoint("Verify: The user is brought to " + userAccount + "'s wishlist page"); // 7
        logFlowPoint("For " + userAccount + " Open the Account and Billings page in a browser"); // 8
        logFlowPoint("Set Privacy to No One, and update"); // 9
        logFlowPoint("Navigate to the wishlist and verify a message"
                + " appears stating the wishlist cannot be viewed appears"); // 10
        logFlowPoint("Refresh the page on " + friendAccount
                + " and verify there is a message stating the profile is private"); // 11
        // 1
        logPassFail(MacroLogin.startLogin(driver, user), true);

        // 2
        logPassFail(MacroWishlist.addToWishlist(driver, entitlement), true);

        // 3
        logPassFail(Waits.pollingWaitEx(() -> MacroWishlist.navigateToWishlist(driver)), true);

        // 4
        String wishlistAddr = MacroWishlist.getWishlistShareAddress(driver);
        logPassFail(wishlistAddr != null, true);

        // 5
        WebDriver browserDriver = null;
        boolean loggedIn = false;
        try {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.setPrivacyThroughBrowserLogin(browserDriver, user, PrivacySetting.EVERYONE);
            browserDriver.get(wishlistAddr);
            loggedIn = MacroLogin.startLogin(browserDriver, friend);
        } catch (WebDriverException e) {
            // org.openqa.selenium.WebDriverException: chrome not reachable
            // or, no session exception
            log(e.getMessage());
        }
        logPassFail(loggedIn, true);

        // 6
        logPassFail(Waits.pollingWaitEx(() -> new ProfileHeader(driver).verifyUsername(user.getUsername())), false);

        // 7
        logPassFail(new ProfileWishlistTab(browserDriver).verifyTilesExist(entitlement.getOfferId()), true);

        // 8
        client.getCompositeDriver().quitBrowserWebDriver();
        WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        logPassFail(Waits.pollingWait(() -> MacroAccount.loginToAccountPage(settingsDriver, user)), true);

        // 9
        logPassFail(Waits.pollingWait(() -> MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, user, PrivacySetting.NO_ONE)), false);

        // 10
        // We need to reload the page to view the new status
        boolean pageRefreshed = Waits.pollingWait(() -> new MainMenu(driver).verifyPageRefreshSuccessful());
        boolean wishlistTabLoaded = false;
        try {
            new ProfileHeader(driver).waitForWishlistTabToLoad();
            wishlistTabLoaded = true;
        } catch (TimeoutException e) {
            log(e.getMessage());
        }
        boolean privateNotificationVerified = Waits.pollingWait(() -> new ProfileWishlistTab(driver).verifyProfilePrivateNotification());
        logPassFail(pageRefreshed && wishlistTabLoaded && privateNotificationVerified, true);

        // 11
        client.getCompositeDriver().quitBrowserWebDriver();
        WebDriver driver2 = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        loggedIn = false;
        boolean isProfilePrivate = false;
        try {
            driver2.get(wishlistAddr);
            loggedIn = Waits.pollingWaitEx(() -> MacroLogin.startLogin(driver2, friend));
            isProfilePrivate = Waits.pollingWaitEx(() -> new ProfileHeader(driver2).verifyProfileIsPrivate());
        } catch (WebDriverException e) {
            log(e.getMessage());
        }
        logPassFail(loggedIn && isProfilePrivate, true);

        softAssertAll();
    }
}
