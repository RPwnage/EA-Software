package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import static com.ea.originx.automation.lib.macroaction.MacroGameLibrary.finishDownloadingEntitlement;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.scripts.client.OADownloadLocationChange;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.utils.SystemUtilities;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Test multiple hardware supports feature
 * 
 * @author sunlee
 */
public class OAChangeLocationInitiatingDownload extends EAXVxTestTemplate{
    private static final Logger _log = LogManager.getLogger(OADownloadLocationChange.class);

    private String NEW_INSTALL_LOCATION = null;
    private OriginClient CLIENT = null;
    
    @TestRail(caseId = 2649034)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testChangeLocationInitiatingDownload(ITestContext context) throws Exception {

        CLIENT = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Launch Origin and login to Origin with an account that owns an entitlement"); // 1
        logFlowPoint("Navigate to 'Game Library'"); // 2
        logFlowPoint("Start to download the entitlement at a valid alternate location"); // 3
        logFlowPoint("Download and install the entitlement"); // 4
        logFlowPoint("Verify DiP Small is installed to the location given"); // 5

        //1
        WebDriver driver = startClientObject(context, CLIENT);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        //3
        NEW_INSTALL_LOCATION = SystemUtilities.getProgramFilesPath(CLIENT) + "\\NewInstallLocation";
        FileUtil.deleteFolder(CLIENT, NEW_INSTALL_LOCATION);
        FileUtil.createFolder(CLIENT, NEW_INSTALL_LOCATION);
        logPassFail(FileUtil.isDirectoryExist(CLIENT, NEW_INSTALL_LOCATION) &&
                MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferId, new DownloadOptions().setChangeDownloadLocation(NEW_INSTALL_LOCATION)), true);
        
        //4
        logPassFail(finishDownloadingEntitlement(driver, entitlementOfferId), true);
        
        //5
        String entitlmentExecutableNameWithPath = NEW_INSTALL_LOCATION + "\\" + entitlement.getDirectoryName() + "\\" + entitlement.getProcessName();
        logPassFail(FileUtil.isFileExist(CLIENT, entitlmentExecutableNameWithPath), true);

        softAssertAll();
    }
    
    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (null != NEW_INSTALL_LOCATION && null != CLIENT) {
            FileUtil.deleteFolder(CLIENT, NEW_INSTALL_LOCATION);
        }

        _log.debug("Performing test cleanup for " + this.getClass().getSimpleName());
        super.testCleanUp(result, context);
    }

}