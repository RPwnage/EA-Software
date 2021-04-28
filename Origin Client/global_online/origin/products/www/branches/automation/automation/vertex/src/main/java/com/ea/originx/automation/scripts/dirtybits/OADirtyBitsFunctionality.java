package com.ea.originx.automation.scripts.dirtybits;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;

import static com.ea.originx.automation.lib.resources.OriginClientData.CLIENT_LOG;
import static com.ea.originx.automation.lib.resources.OriginClientData.LOG_PATH;

import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.OriginClientLogReader;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests connection to the Dirty Bits server for client
 */
public class OADirtyBitsFunctionality extends EAXVxTestTemplate {

    @TestRail(caseId = 1016733)
    @Test(groups = {"client_only", "client", "services_smoke"})
    public void testDirtyBitsFunctionality(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Enable EACore.ini settings for logging dirty bit viewer and Login to Origin with a newly created account"); //1
        logFlowPoint("Select Dirty Bits connect sample handler from debug menu and verify connection to the Dirty Bit server"); //1

        //1
        EACoreHelper.setShowDebugMenuSetting(client.getEACore());
        EACoreHelper.setLogDirtyBitsSetting(client.getEACore());
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Enabled EACore.ini settings for logging Dirty Bits and logged into new throwaway user account " + userAccount.getUsername());
        } else {
            logFailExit("Failed to create and log into new throwaway user account");
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectDirtyBitsConnectSampleHandlersMenu();
        final OriginClientLogReader logReader = new OriginClientLogReader(LOG_PATH);
        if (!"".equals(logReader.retriveLogLine(client, CLIENT_LOG, "DirtyBits client Connected."))) {
            logPass("Selected Dirty Bits connect sample handler from debug menu and verified connection to the Dirty Bits server");
        } else {
            logFail("Failed: unable to verify connection to the Dirty Bits Server");
        }

        softAssertAll();
    }
}
