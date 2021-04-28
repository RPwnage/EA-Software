package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.AboutDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.OriginErrorReporterDialog;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 *  Tests the items present in Help Menu
 *
 * @author cbalea
 */
public class OAMenuHelp extends EAXVxTestTemplate{
    
    @TestRail(caseId = 9762)
    @Test(groups = {"client", "client_only", "full_regression", "long_smoke"})
    public void testMenuHelp (ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getRandomAccount();
        
        logFlowPoint("Launch and log into Origin"); //1
        logFlowPoint("Verify 'Origin Help' is in the Help drop down and clicking it launches the Origin help page in an external browser"); //2
        logFlowPoint("Verify 'Origin Forum' is in the Help drop down and clicking it lauches a page in a external browser"); //3
        logFlowPoint("Verify 'Origin Error Reporter' is in the Help drop down and clicking it opens the Origin Error Reporter"); //4
        logFlowPoint("Verify 'About' is in the Help drop down and clicking it displays a dialog that lists version, release date and other information"); //5
        
        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        MainMenu mainMenu = new MainMenu(driver);
        boolean originHelp = mainMenu.verifyItemExistsInHelp("Origin Help");
        mainMenu.selectOriginHelp();
        boolean originHelpPage = Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client));
        TestScriptHelper.killBrowsers(client);      
        logPassFail(originHelp && originHelpPage, false);
        
        //3
        boolean originForum = mainMenu.verifyItemEnabledInHelp("Origin Forum");
        mainMenu.selectOriginForum();
        boolean originForumPage = Waits.pollingWaitEx(() -> TestScriptHelper.verifyBrowserOpen(client));
        TestScriptHelper.killBrowsers(client);        
        logPassFail(originForum && originForumPage, false);
        
        //4
        boolean errorReporter = mainMenu.verifyItemEnabledInHelp("Origin Error Reporter");
        mainMenu.selectOriginErrorReporter();
        OriginErrorReporterDialog originErrorReporterDialog = new OriginErrorReporterDialog(driver);
        boolean errorReporterDialog = originErrorReporterDialog.verifyOriginErrorReporterRunning() && originErrorReporterDialog.waitIsVisible();   
        logPassFail(errorReporter && errorReporterDialog, true);
        
        //5
        originErrorReporterDialog.killOriginErrorReporterProcess();
        boolean originAbout = mainMenu.verifyItemEnabledInHelp("About");
        mainMenu.selectAbout();
        AboutDialog aboutDialog = new AboutDialog(driver);
        boolean version = aboutDialog.verifyVersionText();
        boolean releaseNotes = aboutDialog.waitReleaseNotesLinkVisible();  
        logPassFail(originAbout && version && releaseNotes, true);
    }
}