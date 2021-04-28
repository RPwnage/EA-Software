package com.ea.originx.automation.scripts.client;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.InvalidDirectoryDialog;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;

import static com.ea.vx.originclient.templates.OATestBase.sleep;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;

import java.awt.AWTException;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that certain directories are invalid when setting the Game Library
 * download location in the Installs & Saves settings
 *
 * @author lscholte
 */
public class OAInvalidLocation extends EAXVxTestTemplate {

    @TestRail(caseId = 702055)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testInvalidLocation(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final String invalidLocation1 = "C:\\Program Files\\";
        final String invalidLocation2 = "C:\\Program Files (x86)\\";
        final String invalidLocation3 = "C:\\Windows\\";

        UserAccount user = AccountManager.getRandomAccount();

        logFlowPoint("Log into Origin as " + user.getUsername()); //1
        logFlowPoint("Navigate to the 'Application Settings' page"); //2
        logFlowPoint("Navigate to the 'Installs & Saves' section"); //3
        logFlowPoint("Verify that the 'Invalid Directory' dialog appears when changing the download folder to " + invalidLocation1); //4
        logFlowPoint("Close the dialog and verify that the download location remains unchanged"); //5
        logFlowPoint("Verify that the 'Invalid Directory' dialog appears when changing the download folder to " + invalidLocation2); //6
        logFlowPoint("Close the dialog and verify that the download location remains unchanged"); //7
        logFlowPoint("Verify that the 'Invalid Directory' dialog appears when changing the download folder to " + invalidLocation3); //8
        logFlowPoint("Close the dialog and verify that the download location remains unchanged"); //9

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        logPassFail(appSettings.verifyAppSettingsReached(), true);

        //3
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);
        logPassFail(Waits.pollingWait(() -> installSaveSettings.verifyInstallSaveSettingsReached()), true);

        //4
        installSaveSettings.clickChangeGameFolderLocation();
        typeNewFolderLocation(client, invalidLocation1);
        InvalidDirectoryDialog invalidDirectoryDialog = new InvalidDirectoryDialog(driver);
        logPassFail(invalidDirectoryDialog.isOpen(), true);

        //5
        invalidDirectoryDialog.close();
        Waits.pollingWait(() -> !invalidDirectoryDialog.isOpen());
        logPassFail(installSaveSettings.verifyGameLibraryLocation(SystemUtilities.getOriginGamesPath(client)), true);

        //6
        installSaveSettings.clickChangeGameFolderLocation();
        typeNewFolderLocation(client, invalidLocation2);
        logPassFail(invalidDirectoryDialog.isOpen(), true);

        //7
        invalidDirectoryDialog.close();
        Waits.pollingWait(() -> !invalidDirectoryDialog.isOpen());
        logPassFail(installSaveSettings.verifyGameLibraryLocation(SystemUtilities.getOriginGamesPath(client)), true);

        //8
        installSaveSettings.clickChangeGameFolderLocation();
        typeNewFolderLocation(client, invalidLocation3);
        logPassFail(invalidDirectoryDialog.isOpen(), true);

        //9
        invalidDirectoryDialog.close();
        Waits.pollingWait(() -> !invalidDirectoryDialog.isOpen());
        logPassFail(installSaveSettings.verifyGameLibraryLocation(SystemUtilities.getOriginGamesPath(client)), true);

        softAssertAll();
    }

    private void typeNewFolderLocation(OriginClient client, String location) throws AWTException, InterruptedException {
        try {
            RobotKeyboard robotHandler = new RobotKeyboard();
            robotHandler.type(client, location, 200);

            sleep(1000);
            robotHandler.type(client, "\n\n", 2000);
        } catch (Exception e) {
            // If the robot fails, the check in the parent's function
            // will catch it
        }
    }

}
