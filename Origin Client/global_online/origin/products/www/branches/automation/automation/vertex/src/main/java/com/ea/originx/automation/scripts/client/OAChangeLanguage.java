package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileGamesTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.ConfirmLanguageDialog;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the ability of the client to change it's language
 *
 * @author jmittertreiner
 * @author caleung
 */
public class OAChangeLanguage extends EAXVxTestTemplate {

    @TestRail(caseId = 10060)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testChangeLanguage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST("Canada");

        /*
        todo: Add these to the german i18n file and update this script to use these from the i18n file
         */
        String loginTextGerman = "Melde dich mit deinem EA-Konto an";
        String redeemCodeGerman = "Produktcode einlösen ...";
        String freeGamesGerman = "Gratis-Spiele bei Origin";
        String appSettingsGerman = "Anwendungseinstellungen";
        String noGamesGerman = "Du hast noch keine Spiele.";

        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Navigate to 'Application Settings'."); // 2
        logFlowPoint("Click on the 'Language' dropdown menu and verify that each language option is localized."); // 3
        logFlowPoint("Choose a non-English language from the list."); // 4
        logFlowPoint("Select 'Cancel' on the 'Restart to Apply Changes' pop up and verify that cancelling a language doesn't " +
                "change the language."); // 5
        logFlowPoint("Choose a different non-English language and click 'Restart Now' on the pop up window."); // 6
        logFlowPoint("Log back into Origin and verify user can log into Origin in online mode after changing the client " +
                "language to a non-English language."); // 7
        logFlowPoint("Verify that that the text on the 'My Games', 'Application Settings', and 'My Profile' pages" +
                " are localized."); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with user account " + userAccount.getUsername());
        } else {
            logFailExit("Failed to log into Origin with user account " + userAccount.getUsername());
        }

        // 2
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        if (appSettings.verifyAppSettingsReached()) {
            logPass("Successfully navigated to the 'Application Settings' page.");
        } else {
            logFailExit("Failed to navigate to the 'Application Settings' page.");
        }

        // 3
        LanguageEnum currLanguage = appSettings.getCurrentLanguage(); // save current language setting for later
        if (appSettings.verifyAllLanguagesLocalized()) {
            logPass("Verified that each language option in the dropdown is localized.");
        } else {
            logFail("Failed to verify that each language option in the dropdown is localized.");
        }

        // 4
        String mainWindow = driver.getWindowHandle();
        appSettings.selectLanguage(LanguageEnum.FRENCH);
        ConfirmLanguageDialog dialog = new ConfirmLanguageDialog(driver);
        if (dialog.waitIsVisible()) {
            logPass("Successfully selected a non-English language from the dropdown.");
        } else {
            logFail("Failed to select a non-English language from the dropdown.");
        }

        // 5
        dialog.cancel();
        if (StringHelper.containsIgnoreCase(appSettings.getCurrentLanguage().getName(), currLanguage.getName())
                && StringHelper.containsIgnoreCase(appSettings.getAppSettingsHeaderText(), "Application Settings")) {
            logPass("Verified that cancelling a language doesn't change the language.");
        } else {
            logFail("Failed to verify that cancelling a language doesn't change the language.");
        }
        driver.switchTo().window(mainWindow);

        // 6
        appSettings.selectLanguage(LanguageEnum.GERMAN);
        dialog.restart();
        // Clicking the restart button on the dialog completely kills Origin,
        // bringing the driver down with it, killing the test. To get around
        // that, we kill Origin manually and relaunch it.
        ProcessUtil.killProcess(client, "Origin*");
        client.stop();

        driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        boolean isLoginPage = Waits.pollingWait(() -> loginPage.isPageThatMatchesOpen(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Successfully restarted Origin by changing the client language.");
        } else {
            logFailExit("Failed to restart Origin by changing the client language.");
        }

        // 7
        boolean isLanguageCorrect = loginPage.verifyTitleIs(loginTextGerman);
        if (isLanguageCorrect && MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified the user can successfully log into Origin after changing the language.");
        } else {
            logFailExit("Failed to verify the user can log into Origin after changing the language.");
        }

        // 8
        MainMenu mainMenu = new MainMenu(driver);
        boolean isMainMenuLocalized = mainMenu.verifyItemEnabledInOrigin(redeemCodeGerman);
        appSettings = mainMenu.selectApplicationSettings();
        boolean isAppSettingsLocalized = StringHelper.containsIgnoreCase(appSettings.getAppSettingsHeaderText(), appSettingsGerman);
        mainMenu.selectMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openGamesTab("Spiele");
        boolean isMyGamesLocalized = StringHelper.containsIgnoreCase(new ProfileGamesTab(driver).getNoGamesMessageText(), noGamesGerman);
        if (isMainMenuLocalized && isAppSettingsLocalized && isMyGamesLocalized) {
            logPass("Verified that the text on the 'My Games', 'Application Settings', and 'My Profile' pages are" +
                    " localized.");
        } else {
            logFail("Failed to verify that the text on the 'My Games', 'Application Settings', and 'My Profile' " +
                    "pages are localized.");
        }

        softAssertAll();
    }
}