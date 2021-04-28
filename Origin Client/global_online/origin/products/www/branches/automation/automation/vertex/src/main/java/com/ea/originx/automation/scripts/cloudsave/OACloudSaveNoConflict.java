package com.ea.originx.automation.scripts.cloudsave;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.TestCleaner;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test cloud save feature while there should be no conflict.
 * @author sunlee
 */
public class OACloudSaveNoConflict extends EAXVxTestTemplate {
    @TestRail(caseId = 3866981)
    @Test(groups = {"services_smoke", "client_only", "cloudsaves", "int_only"})
    public void testCloudSaveNoConflict(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.ENTITLEMENT_1103); // Use 1103 entitlement as this is testing cloudsave functionality

        EntitlementInfo cloudSaveEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BEJEWELED3);
        String cloudSaveOfferId = cloudSaveEntitlement.getOfferId();

        //clear the local app data
        new TestCleaner(client).clearLocalSettings(true);
        
        logFlowPoint("Launch Origin and sign into an account owns a cloudsave enabled entitlement");//1
        logFlowPoint("Download the cloud save entitlement"); //2
        logFlowPoint("Launch the game with cloud saves and verify the game launches without conflict"); //3
        logFlowPoint("Exit the game and verify the game exits without cloud conflict");//4
        logFlowPoint("Launch the game with cloud saves and verify the game launches without conflict");//5
        logFlowPoint("Exit the game and verify the game exits without cloud conflict");//6
        
        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, cloudSaveOfferId), true);
        
        //3
        TestScriptHelper.setCloudSaveEnabled(driver, true);
        TestScriptHelper.setIGOEnabled(driver, false);
        cloudSaveEntitlement.addActivationFile(client);
        GameTile gameTile = new GameTile(driver, cloudSaveOfferId);
        gameTile.play();
        Waits.sleep(3000); // Wait game to be stable
        logPassFail(cloudSaveEntitlement.isLaunched(client), true);

        //4
        cloudSaveEntitlement.killLaunchedProcess(client);
        Waits.sleep(30000); // Wait game exits properly
        logPassFail(gameTile.waitForReadyToPlay(30), true);

        //5
        gameTile.play();
        Waits.sleep(3000); // Wait game to be stable
        logPassFail(cloudSaveEntitlement.isLaunched(client), true);

        //6
        cloudSaveEntitlement.killLaunchedProcess(client);
        Waits.sleep(30000); // Wait game exits properly
        logPassFail(gameTile.waitForReadyToPlay(30), true);
        
        softAssertAll();
    }
}
