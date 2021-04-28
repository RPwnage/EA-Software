package com.ea.originx.automation.scripts.feature.originaccess;

import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelMembership;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationConfirmed;
import com.ea.originx.automation.lib.pageobjects.dialog.CancellationSurveyDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to Upgrade and Remove an Upgrade from a Vault Entitlement
 *
 * @author glivingstone
 */
public class OADownloadAfterCancelling extends EAXVxTestTemplate {

    @TestRail(caseId = 3103325)
    @Test(groups = {"gamelibrary", "originaccess_smoke", "originaccess", "int_only", "client_only", "allfeaturesmoke"})
    public void testDownloadAfterCancelling(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo bf4Ent = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String entName = bf4Ent.getName();
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Log into Origin as newly registered account"); // 1
        logFlowPoint("Purchase an Origin Access Subscription"); // 2
        logFlowPoint("Cancel the Origin Access Subscription"); // 3
        logFlowPoint("Navigate to the Vault Page for Origin Access"); // 4
        logFlowPoint("Click 'Get it Now' on a Vault Entitlement"); // 5
        logFlowPoint("Click 'Download Now' in the Confirmation Dialog and Complete the Download Dialogs"); // 6
        logFlowPoint("Verify the Entitlement is Downloading"); // 7
        logFlowPoint("Verify the Entitlement Appears in the Game Library"); // 8

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);

        // 3
        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        accountSettingsPage.gotoOriginAccessSection();
        OriginAccessSettingsPage originAccessSettingsPage = new OriginAccessSettingsPage(browserDriver);
        originAccessSettingsPage.cancelMembership();
        new CancelMembership(browserDriver).clickContinueCTA();
        new CancellationSurveyDialog(browserDriver).clickCompleteCancellationCTA();
        logPassFail(new CancellationConfirmed(browserDriver).isDialogVisible(), true);
        browserDriver.close();

        // 4
        NavigationSidebar navBar = new NavigationSidebar(driver);
        navBar.gotoOriginAccessPage();
        VaultPage vaultPage = new VaultPage(driver);
        logPassFail(Waits.pollingWait(() -> vaultPage.verifyPageReached()), true);

        // 5
        MacroGDP.loadGdpPage(driver, bf4Ent);
        GDPHeader gdpHeader = new GDPHeader(driver);
        gdpHeader.verifyGDPHeaderReached();
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.isDialogVisible(), true);

        // 6
        checkoutConfirmation.clickDownload();
        DownloadOptions optionsNoExtraContent = new DownloadOptions().setDesktopShortcut(false).setStartMenuShortcut(false).setUncheckExtraContent(true);
        logPassFail(MacroGameLibrary.handleDialogs(driver, bf4Ent.getOfferId(), optionsNoExtraContent), true);

        // 7
        logPassFail(new GameTile(driver, bf4Ent.getOfferId()).isDownloading(), true);

        // 8
        logPassFail(new GameLibrary(driver).isGameTileVisibleByName(entName), true);

        softAssertAll();
    }
}