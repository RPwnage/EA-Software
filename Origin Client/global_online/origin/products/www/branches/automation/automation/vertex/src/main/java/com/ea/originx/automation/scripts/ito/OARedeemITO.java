package com.ea.originx.automation.scripts.ito;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroRedeem;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemCompleteDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.ITODisc;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test downloading and installing a game from an ITO Disc
 *
 * @author palui
 */
public class OARedeemITO extends EAXVxTestTemplate {

    @TestRail(caseId = 9311)
    @Test(groups = {"ito", "client_only", "full_regression"})
    public void testRedeemITO(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        String entitlementName = entitlement.getName();
        final String offerId = entitlement.getOfferId();
        final String productCode = entitlement.getProductCode();

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as newly registered user: " + username);//1
        logFlowPoint("Navigate to the 'Game Library' page");//2
        logFlowPoint(String.format("Insert ITO disc for '%s'. Verify 'Redeem' dialog opens for '%s'", entitlementName, entitlementName));//3
        logFlowPoint(String.format("Redeem '%s'", entitlementName));//4
        logFlowPoint(String.format("Verify '%s' game tile appears in the game library", entitlementName));//5
        logFlowPoint(String.format("Download and install '%s' game from the ITO Disc", entitlementName));//6
        logFlowPoint(String.format("Launch '%s' game", entitlementName));//7

        //1
        final WebDriver driver = startClientObject(context, client);
        GameTile gameTile = new GameTile(driver, offerId);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        // Navigate to Game Library first as current macro for downloading checks the game tile
        // and once the install modal dialog appears we cannot use the navigation sidebar
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageEmpty(), false);

        //3
        new ITODisc().insertDisc(client);
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.waitAndSwitchToRedeemProductDialog();
        redeemDialog.waitForVisible();
        logPassFail(redeemDialog.isOpenForActivation(entitlementName), true);

        //4
        logPassFail(MacroRedeem.redeemProductCode(driver, productCode, entitlementName), false);

        //5
        new RedeemCompleteDialog(driver).clickLaunchGameButton();
        // forcing a refresh now because 'Game library' isn't automatically refresh 
        new MainMenu(driver).selectRefresh();
        logPassFail(gameTile.isGameTileVisible(), true);

        //6
        // Download has already been initiated and the 'Download Game' message box should be visible.
        // No need to initiate download from the game tile's context menu
        DownloadOptions options = new DownloadOptions().setDownloadTrigger(DownloadOptions.DownloadTrigger.INSTALL_FROM_DISC);
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, offerId, options), false);

        //7
        gameTile.play();
        logPassFail(entitlement.isLaunched(client), true);

        softAssertAll();
    }
}
