package com.ea.originx.automation.scripts.client;

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
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author glivingstone
 */
public class OABasicSettings extends EAXVxTestTemplate {

    @TestRail(caseId = 3068173)
    @Test(groups = {"client", "quick_smoke", "long_smoke", "client_only"})
    public void testBasicSettings(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch Origin and login as user: " + userAccount.getUsername()); //1
        logFlowPoint("Click 'Diagnostics' link at the 'Settings' navigation bar");//2
        logFlowPoint("Click 'Installs & Saves' link at the 'Settings' navigation bar");//3
        logFlowPoint("Click 'Notifications' link at the 'Settings' navigation bar");//4
        logFlowPoint("Click 'Origin In-Game' link at the 'Settings' navigation bar");//5
        logFlowPoint("Click 'Voice' link at the 'Settings' navigation bar"); //6
        logFlowPoint("Click 'Application Settings' at the 'Origin' dropdown"); //7

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful to user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login to user: " + userAccount.getUsername());
        }

        //2
        new MainMenu(driver).selectApplicationSettings();
        SettingsNavBar navBar = new SettingsNavBar(driver);
        navBar.clickDiagnosticsLink();
        DiagSettings diagSettings = new DiagSettings(driver);

        if (Waits.pollingWait(() -> diagSettings.verifyDiagSettingsReached())) {
            logPass("Verified 'Settings - Dignostics' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Dignostics' page");
        }

        //3
        navBar.clickInstallSaveLink();
        InstallSaveSettings isSettings = new InstallSaveSettings(driver);

        if (Waits.pollingWait(() -> isSettings.verifyInstallSaveSettingsReached())) {
            logPass("Verified 'Settings - Installs & Saves' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Installs & Saves' page");
        }

        //4
        navBar.clickNotificationsLink();
        NotificationsSettings notificationSettings = new NotificationsSettings(driver);
        if (Waits.pollingWait(() -> notificationSettings.verifyNotificationsSettingsReached())) {
            logPass("Verified 'Settings - Notifications' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Notifications' page");
        }

        //5
        navBar.clickOIGLink();
        OIGSettings oigSettings = new OIGSettings(driver);
        if (Waits.pollingWait(() -> oigSettings.verifyOIGSettingsReached())) {
            logPass("Verified 'Settings - Origin In-Game' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Origin In-Game' page");
        }

        //6
        navBar.clickVoiceLink();
        VoiceSettings voiceSettings = new VoiceSettings(driver);
        if (Waits.pollingWait(() -> voiceSettings.verifyVoiceSettingsReached())) {
            logPass("Verified 'Settings - Voice' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Voice' page");
        }

        //7
        navBar.clickApplicationLink();
        AppSettings appSettings = new AppSettings(driver);
        if (Waits.pollingWait(() -> appSettings.verifyAppSettingsReached())) {
            logPass("Verified 'Settings - Application' page opens successfully");
        } else {
            logFailExit("Failed: Cannot open 'Settings - Application' page");
        }

        softAssertAll();
    }
}
