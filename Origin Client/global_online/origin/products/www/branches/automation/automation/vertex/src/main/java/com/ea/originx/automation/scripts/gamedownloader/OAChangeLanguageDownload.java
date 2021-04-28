package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.ConfirmLanguageDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import java.util.Locale;

/**
 * Test that downloads complete after changing the client language.
 *
 * @author jmittertreiner
 */
public class OAChangeLanguageDownload extends EAXVxTestTemplate {

    @TestRail(caseId = 9880)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testChangeLanguageDownload(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        I18NUtil.setLocale(new Locale("fr", "ca"));
        I18NUtil.registerResourceBundle("i18n.MessagesBundle");

        logFlowPoint("Launch Origin and login as user."); // 1
        logFlowPoint("Change language to 'French' and restart Origin."); // 2
        logFlowPoint("Login to Origin again and navigate to the 'Game Library'."); // 3
        logFlowPoint("Click download on " + entitlement.getName() + " and stop at the language dialog."); // 4
        logFlowPoint("Cancel the download and verify the download is cancelled."); // 5
        logFlowPoint("Download " + entitlement.getName() + " and stop at the EULA."); // 6
        logFlowPoint("Cancel the download and verify the download is cancelled."); // 7
        logFlowPoint("Completely download " + entitlement.getName() + " and verify the download completes."); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        //AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        new MainMenu(driver).selectApplicationSettings().selectLanguage(LanguageEnum.FRENCH);
        ConfirmLanguageDialog confirmLanguageDialog = new ConfirmLanguageDialog(driver);
        confirmLanguageDialog.waitForVisible();
        confirmLanguageDialog.restart();
        Waits.pollingWaitEx(() -> !ProcessUtil.isOriginRunning(client));
        logPassFail(Waits.pollingWaitEx(() -> ProcessUtil.isOriginRunning(client)), true);
        client.stop();
        ProcessUtil.killOriginProcess(client);
        Waits.pollingWaitEx(() -> !ProcessUtil.isOriginRunning(client));

        // 3
        driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, userAccount);
        //GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        boolean isGameLibraryReached = new NavigationSidebar(driver).gotoGameLibrary().verifyGameLibraryPageReached();
        logPassFail(isGameLibraryReached, true);

        // 4
        GameTile gameTile = new GameTile(driver, entitlement.getOfferId());
        gameTile.startDownload();
        DownloadLanguageSelectionDialog languageDialog = new DownloadLanguageSelectionDialog(driver);
        logPassFail(languageDialog.waitIsVisible(), true);

        // 5
        languageDialog.clickCloseCircle();
        logPassFail(Waits.pollingWait(() -> !gameTile.isDownloading()), true);

        // 6
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, entitlement.getOfferId(), new DownloadOptions().setStopAtEULA(true)), true);

        // 7
        new DownloadEULASummaryDialog(driver).closeAndWait();
        logPassFail(Waits.pollingWait(() -> !gameTile.isDownloading()), true);

        // 8
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entitlement.getOfferId()), true);

        softAssertAll();
    }
}