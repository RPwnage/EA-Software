package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CancelDownload;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that games are downloaded into the correct folder inside the Origin
 * Games folder
 *
 * @author lscholte
 */
public class OADownloadFileVerification extends EAXVxTestTemplate {

    @TestRail(caseId = 9877)
    @Test(groups = {"client", "gamedownloader", "client_only", "full_regression"})
    public void testDownloadFileVerification(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String username = userAccount.getUsername();
        String downloadPath = SystemUtilities.getOriginGamesPath(client) + "\\" + entitlementName;
        
        logFlowPoint("Log into Origin as " + username); //1
        logFlowPoint("Navigate to the 'Game Library' page"); //2
        logFlowPoint("Start downloading " + entitlementName); //3
        logFlowPoint("Verify a folder for " + entitlementName + " exists in the Origin Games folder"); //4
        logFlowPoint("Cancel the download"); //5
        logFlowPoint("Verify a folder for " + entitlementName + " no longer exists in the Origin Games folder"); //6
        logFlowPoint("Start downloading " + entitlementName); //7
        logFlowPoint("Verify a folder for " + entitlementName + " exists in the Origin Games folder"); //8

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId), true);

        //4
        logPassFail(FileUtil.isDirectoryExist(client, downloadPath), true);

        //5
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        gameTile.cancelDownload();
        new CancelDownload(driver).clickYes();
        logPassFail(Waits.pollingWait(() -> !gameTile.isDownloading()), true);

        //6
        logPassFail(!FileUtil.isDirectoryExist(client, downloadPath), true);

        //7
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId), true);

        //8
        logPassFail(FileUtil.isDirectoryExist(client, downloadPath), true);

        softAssertAll();
    }
}
