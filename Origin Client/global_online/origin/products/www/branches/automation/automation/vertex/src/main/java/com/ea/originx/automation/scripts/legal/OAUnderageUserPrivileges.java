package com.ea.originx.automation.scripts.legal;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.settings.NotificationsSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;

import java.util.Arrays;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author lscholte
 */
public class OAUnderageUserPrivileges extends EAXVxTestTemplate {

    @TestRail(caseId = 9594)
    @Test(groups = {"legal", "client_only", "full_regression"})
    public void testUnderageRegisterOriginId(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.createNewThrowAwayAccount(12);

        logFlowPoint("Register and login as an underage user " + user.getUsername()); //1
        logFlowPoint("Verify the user cannot access the Discover page, which contains the New User Experience"); //2
        logFlowPoint("Verify the Friends menu does not exist"); //3
        logFlowPoint("Verify the Social Hub does not exist"); //4
        logFlowPoint("Verify the underage user does not have a profile picture in the mini profile"); //5
        logFlowPoint("Verify the user cannot access the Store page"); //6
        logFlowPoint("Verify the user cannot access the Accounts and Billing page"); //7
        logFlowPoint("Navigate to the Notifications settings page"); //8
        logFlowPoint("Verify there are no settings for Chat or Friend Activity"); //9

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully registered and signed in as the underage user " + user.getUsername());
        } else {
            logFailExit("Failed to register and sign in as the underage user " + user.getUsername());
        }

        //2
        DiscoverPage discoverPage = new DiscoverPage(driver);
        NavigationSidebar navBar = new NavigationSidebar(driver);
        boolean notOnDiscoverPage = !discoverPage.verifyDiscoverPageReached();
        boolean discoverPageLinkNotVisible = !navBar.verifyDiscoverLinkVisible();
        if (notOnDiscoverPage && discoverPageLinkNotVisible) {
            logPass("An underage user is unable to access the Discover page");
        } else {
            logFail("An underage user is able to access the Discover page");
        }

        //3
        MainMenu mainMenu = new MainMenu(driver);
        if (!mainMenu.verifyMenuExists("Friends")) {
            logPass("The Friends menu does not exist for an underage user");
        } else {
            logFail("The Friends menu exists for an underage user");
        }

        //4
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 10);
        SocialHub socialHub = new SocialHub(driver);
        SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
        boolean socialHubNotVisible = !socialHub.verifySocialHubVisible() && !socialHubMinimized.verifyMinimized();
        if (socialHubNotVisible) {
            logPass("The Social Hub does not exist for an underage user");
        } else {
            logFail("The Social Hub exists for an underage user");
        }

        //5
        MiniProfile miniProfile = new MiniProfile(driver);
        if (!miniProfile.verifyAvatarVisible()) {
            logPass("An underage user does not have a visible profile picture");
        } else {
            logFail("An underage user has a visible profile picture");
        }

        //6
        if (!navBar.verifyStoreLinkVisible()) {
            logPass("The store link is not accessible to an underage user");
        } else {
            logFail("The store link is accessible to an underage user");
        }

        //7
        boolean miniProfileNotHaveAccountsAndBilling = Waits.pollingWait(() -> miniProfile.verifyPopoutMenuItemsExist(
                Arrays.asList("EA Account and Billing"),
                new boolean[]{false}));
        boolean mainMenuNotHaveAccountsAndBilling = Waits.pollingWait(() -> !mainMenu.verifyItemExistsInOrigin("EA Account and Billing\u2026"));
        if (miniProfileNotHaveAccountsAndBilling && mainMenuNotHaveAccountsAndBilling) {
            logPass("An underage user is not able to access EA Account and Billing");
        } else {
            logFail("An underage user is able to access EA Account and Billing");
        }

        //8
        mainMenu.selectApplicationSettings();
        new SettingsNavBar(driver).clickNotificationsLinkUnderage();
        NotificationsSettings notificationsSettings = new NotificationsSettings(driver);
        if (Waits.pollingWait(() -> notificationsSettings.verifyNotificationsSettingsReached())) {
            logPass("Successfully navigated to the notifications settings page");
        } else {
            logFailExit("Failed to navigate to the notifications settings page");
        }

        //9
        boolean chatSectionInvisible = Waits.pollingWait(() -> !notificationsSettings.verifyChatSectionVisible());
        boolean friendsActivitySectionInvisible = Waits.pollingWait(() -> !notificationsSettings.verifyFriendsActivitySectionVisible());
        if (chatSectionInvisible && friendsActivitySectionInvisible) {
            logPass("An underage user is unable to adjust settings for chat and/or friends activity notifications");
        } else {
            logFail("An underage user is able to adjust settings for chat and/or friends activity notifications");
        }

        softAssertAll();
    }
}
