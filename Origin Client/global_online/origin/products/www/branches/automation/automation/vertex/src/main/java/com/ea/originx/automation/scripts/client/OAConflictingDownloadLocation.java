package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the behaviour of conflicting download locations
 *
 * @author jmittertreiner
 */
public class OAConflictingDownloadLocation extends EAXVxTestTemplate {

    @TestRail(caseId = 10070)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testConflictingDownloadLocation(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final AccountManager accountManager = AccountManager.getInstance();
        Criteria criteria = new Criteria.CriteriaBuilder().build();
        UserAccount user = accountManager.requestWithCriteria(criteria, 360);

        logFlowPoint("Login to Origin"); // 1
        logFlowPoint("Navigate to Application Settings -> Installs and Saves"); // 2
        logFlowPoint("Verify: You cannot set the installers to the same location as the downloaded games"); // 4
        logFlowPoint("Verify: You cannot set the downloaded games to the same location as the installers "); // 6

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with " + user.getUsername());
        } else {
            logFailExit("Could not log into Origin with " + user.getUsername());
        }

        // 2
        new MainMenu(driver).selectApplicationSettings();
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings installSaveSettings = new InstallSaveSettings(driver);
        if (installSaveSettings.verifyInstallSaveSettingsReached()) {
            logPass("Successfully navigated to the 'Install Save Settings' page");
        } else {
            logFailExit("Failed to navigate to the 'Install Save Settings' page");
        }

        // 3
        if (installSaveSettings.changeGamesFolderToInvalid(TestScriptHelper.getOriginEntitlementInstallerPath())) {
            logPass("Cannot change download location");
        } else {
            logFail("Download location was able to be changed");
        }

        // 4
        if (installSaveSettings.changeInstallerFolderToInvalid(SystemUtilities.getOriginGamesPath(client))) {
            logPass("Cannot change download location");
        } else {
            logFail("Download location was able to be changed");
        }

        softAssertAll();
    }
}
