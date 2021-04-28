package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.DiagSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.NotificationsSettings;
import com.ea.originx.automation.lib.pageobjects.settings.OIGSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.pageobjects.settings.VoiceSettings;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify that clicking toggle slides on settings pages results in the
 * flashing (i.e. appears then disappears) of notification toast display
 *
 * @author palui
 */
public class OASettingsNotifications extends EAXVxTestTemplate {

    @TestRail(caseId = 10044)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testSettingsNotifications(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Launch Origin and login as user: " + userAccount.getUsername()); //1
        logFlowPoint("Click 'Application Settings' at the 'Origin' dropdown"); //2
        logFlowPoint("Verify notification appears after changing any setting with an ON/OFF button in 'Settings - Application'");//3
        logFlowPoint("Click 'Diagnostics' link at the 'Settings' navigation bar");//4
        logFlowPoint("Verify notification appears after changing any setting with an ON/OFF button in 'Settings - Diagnostics'");//5
        logFlowPoint("Click 'Installs & Saves' link at the 'Settings' navigation bar");//6
        logFlowPoint("Verify notification appears after changing any setting with an ON/OFF button in 'Settings - Installs & Saves'");//7
        logFlowPoint("Click 'Notifications' link at the 'Settings' navigation bar");//8
        logFlowPoint("Verify notification appears after changing any setting with an ON/OFF button in 'Settings - Notifications'");//9
        logFlowPoint("Click 'Origin In-Game' link at the 'Settings' navigation bar");//10
        logFlowPoint("Verify notification appears after changing any setting with an ON/OFF button in 'Settings - Origin In-Game'");//11
        logFlowPoint("Click 'Voice' link at the 'Settings' navigation bar"); //12
        logFlowPoint("Verify notification appears after changing any setting with an ON/OFF button in 'Settings - Voice'");//13

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user: " + userAccount.getUsername());
        }

        //2
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        if (Waits.pollingWait(appSettings::verifyAppSettingsReached)) {
            logPass("Verified 'Settings - Application' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Application' page");
        }

        //3
        sleep(3000); // pause for page to stabilize
        if (appSettings.toggleAppSettingsAndVerifyNotifications()) {
            logPass("Verified notification appears after changing any setting with an ON/OFF button in 'Settings - Application'");
        } else {
            logFailExit("Failed: One or more notification does not appear after changing any setting with an ON/OFF button in 'Settings - Application'");
        }
        // If we leave this setting on, we trigger a beta update notification
        // It breaks the test so we don't want that, so turn it off again
        appSettings.toggleParticipateInOriginClientsBetas();

        //4
        SettingsNavBar navBar = new SettingsNavBar(driver);
        navBar.clickDiagnosticsLink();
        DiagSettings diagSettings = new DiagSettings(driver);

        if (Waits.pollingWait(diagSettings::verifyDiagSettingsReached)) {
            logPass("Verified 'Settings - Dignostics' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Dignostics' page");
        }

        //5
        sleep(3000); // pause for page to stabilize
        if (diagSettings.toggleDiagSettingsAndVerifyNotifications()) {
            logPass("Verified notification appears after changing any setting with an ON/OFF button in 'Settings - Diagnostics'");
        } else {
            logFailExit("Failed: One or more notification does not appear after changing any setting with an ON/OFF button in 'Settings - Diagnostics'");
        }

        //6
        navBar.clickInstallSaveLink();
        InstallSaveSettings isSettings = new InstallSaveSettings(driver);

        if (Waits.pollingWait(isSettings::verifyInstallSaveSettingsReached)) {
            logPass("Verified 'Settings - Installs & Saves' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Installs & Saves' page");
        }

        //7
        sleep(3000); // pause for page to stabilize
        if (isSettings.toggleInstallSaveSettingsAndVerifyNotifications()) {
            logPass("Verified notification appears after changing any setting with an ON/OFF button in 'Settings - Installs & Saves'");
        } else {
            logFailExit("Failed: One or more notification does not appear after changing any setting with an ON/OFF button in 'Settings - Installs & Saves'");
        }

        //8
        navBar.clickNotificationsLink();
        NotificationsSettings notificationSettings = new NotificationsSettings(driver);
        if (Waits.pollingWait(notificationSettings::verifyNotificationsSettingsReached)) {
            logPass("Verified 'Settings - Notifications' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Notifications' page");
        }

        //9
        sleep(3000); // pause for page to stabilize
        if (notificationSettings.toggleNotificationsSettingsAndVerifyNotifications()) {
            logPass("Verified notification appears after changing any setting with an ON/OFF button in 'Settings - Notifications'");
        } else {
            logFailExit("Failed: One or more notification does not appear after changing any setting with an ON/OFF button in 'Settings - Notifications'");
        }

        //10
        navBar.clickOIGLink();
        OIGSettings oigSettings = new OIGSettings(driver);
        if (Waits.pollingWait(oigSettings::verifyOIGSettingsReached)) {
            logPass("Verified 'Settings - Origin In-Game' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Origin In-Game' page");
        }

        //11
        sleep(3000); // pause for page to stabilize
        if (oigSettings.toggleOIGSettingsAndVerifyNotifications()) {
            logPass("Verified notification appears after changing any setting with an ON/OFF button in 'Settings - Origin In-Game'");
        } else {
            logFailExit("Failed: One or more notification does not appear after changing any setting with an ON/OFF button in 'Settings - Origin In-Game'");
        }

        //12
        navBar.clickVoiceLink();
        VoiceSettings voiceSettings = new VoiceSettings(driver);
        if (Waits.pollingWait(voiceSettings::verifyVoiceSettingsReached)) {
            logPass("Verified 'Settings - Voice' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Voice' page");
        }

        //13
        sleep(3000); // pause for page to stabilize
        if (voiceSettings.toggleVoiceSettingsAndVerifyNotifications()) {
            logPass("Verified notification appears after changing any setting with an ON/OFF button in 'Settings - Installs & Saves'");
        } else {
            logFailExit("Failed: One or more notification does not appear after changing any setting with an ON/OFF button in 'Settings - Installs & Saves'");
        }

        softAssertAll();
    }
}
