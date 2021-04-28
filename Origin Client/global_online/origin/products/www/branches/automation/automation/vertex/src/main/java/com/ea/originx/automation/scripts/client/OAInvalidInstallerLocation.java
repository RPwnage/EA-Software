package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.InvalidDirectoryDialog;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests that certain directories are invalid when setting the Legacy Game Installer Location
 * in the Installs & Saves settings
 *
 * @author micwang
 */
public class OAInvalidInstallerLocation extends EAXVxTestTemplate {

    // This is probably only working for window vista and above, not for XP, and other OS
    private static final String EXPECTED_INSTALLER_LOC = "C:\\ProgramData\\Origin\\DownloadCache";

    public void testInvalidInstallerLocation(ITestContext context, String invalidPath) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManager.getRandomAccount();

        logFlowPoint("Login to Origin"); //1
        logFlowPoint("Navigate to the 'Application Settings' page"); //2
        logFlowPoint("Navigate to the 'Installs & Saves' tab"); //3
        logFlowPoint("Verify that the 'Invalid Directory' dialog appears when changing the game installer location to " + invalidPath); //4
        logFlowPoint("Close the pop-up dialog and verify that the game installer location remains unchanged"); //5

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        logPassFail(appSettings.verifyAppSettingsReached(), true);

        //3
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);
        logPassFail(Waits.pollingWait(installSaveSettings::verifyInstallSaveSettingsReached), true);

        //4
        installSaveSettings.clickChangeGameInstallerLocation();
        typeNewFolderLocation(client, invalidPath);
        InvalidDirectoryDialog invalidDirectoryDialog = new InvalidDirectoryDialog(driver);
        logPassFail(invalidDirectoryDialog.waitIsVisible(), true);

        //5
        invalidDirectoryDialog.close();
        Waits.pollingWait(() -> !invalidDirectoryDialog.isOpen());
        logPassFail(installSaveSettings.verifyGameInstallerLocation(EXPECTED_INSTALLER_LOC), true);

        softAssertAll();
    }

    private void typeNewFolderLocation(OriginClient client, String location) {
        try {
            // Temporary workaround, needs better wait solution:
            // this is just add more wait time before inputting location into the open directory window in hope
            // of less failures
            sleep(1000);
            RobotKeyboard robotHandler = new RobotKeyboard();
            robotHandler.type(client, location, 200);
            sleep(2000);
            robotHandler.type(client, "\n\n", 2000);
        } catch (Exception e) {
            // If the robot fails, the check in the parent's function will catch it
            logFailExit(e.getMessage());
        }
    }
}