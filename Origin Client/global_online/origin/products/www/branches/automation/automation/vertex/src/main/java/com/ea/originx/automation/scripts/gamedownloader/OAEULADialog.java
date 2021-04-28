package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;

import java.util.Arrays;
import java.util.List;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the EULA Dialog
 *
 * @author glivingstone
 */
public class OAEULADialog extends EAXVxTestTemplate {

    @TestRail(caseId = 9847)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void OAEULADialog(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        String entitlementName = entitlement.getName();
        String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        final List<String> textProcesses = Arrays.asList("notepad.exe", "notepad++.exe", "winword.exe", "WINWORD.EXE", "word.exe", "wordpad.exe");

        logFlowPoint("Launch and log into Origin"); //1
        logFlowPoint("Select any title you have in Game Library."); //2
        logFlowPoint("Select Download and Verify that the user must accept the EULA(s) for the game."); //3
        logFlowPoint("The entitlement EULA was opened in an external word processor that can print EULA.");  //4
        logFlowPoint("EA Privacy and Cookie Policy appears first and The entitlement EULA appears second on the list.");  //5
        logFlowPoint("Verify that once all the EULAs have been accepted, the download is added to the queue.");  //6

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        gameTile.startDownload();
        DownloadLanguageSelectionDialog languageDialog = new DownloadLanguageSelectionDialog(driver);
        logPassFail(languageDialog.waitIsVisible(), true);

        // 3
        MacroGameLibrary.handleDialogs(driver, entitlementOfferId, new DownloadOptions().setStopAtEULA(true));
        DownloadEULASummaryDialog eulaDialog = new DownloadEULASummaryDialog(driver);
        eulaDialog.waitIsVisible();
        boolean nextDisabled = eulaDialog.verifyNextButtonNotClickable();
        boolean cantContinue = !eulaDialog.tryToClickNextButton();
        logPassFail(nextDisabled && cantContinue, true);

        // 4
        ProcessUtil.killProcesses(client, textProcesses);
        eulaDialog.clickEULALink(1);
        logPassFail(Waits.pollingWaitEx(() -> ProcessUtil.isAnyProcessRunning(client, textProcesses)), false);
        ProcessUtil.killProcesses(client, textProcesses);

        // 5
        logPassFail(eulaDialog.verifyEntitlementEULAFirst() && eulaDialog.verifyEntitlementEULASecond(entitlementName), false);

        // 6
        eulaDialog.setLicenseAcceptCheckbox(true);
        eulaDialog.clickNextButton();
        logPassFail (gameTile.waitForDownloading(), true);
        softAssertAll();
    }
}
