package com.ea.originx.automation.scripts.originaccess.gamelibrary.offlinesupport;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
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
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests playing Vault games offline
 *
 * @author tdhillon
 */
public class OAOfflineLaunchVault extends EAXVxTestTemplate {

        @TestRail(caseId = 2911028)
        @Test(groups = {"gamelibrary", "originaccess_smoke", "originaccess", "client_only", "allfeaturesmoke"})
        public void testOAOfflineLaunchVault(ITestContext context) throws Exception {

            final OriginClient client = OriginClientFactory.create(context);

            final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.PEGGLE);
            final String entName = entitlement.getName();
            final String entOfferID = entitlement.getOfferId();
            final String entProcess = entitlement.getProcessName();
            final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
            EntitlementService.grantEntitlement(userAccount, entOfferID);
            final String username = userAccount.getUsername();

            logFlowPoint("Launch Origin and login as user: " + username); //1
            logFlowPoint("Navigate to game library."); //2
            logFlowPoint(String.format("Completely Download '%s'", entName)); //3
            logFlowPoint(String.format("Launch %s", entName)); //4
            logFlowPoint("Go Offline using the Origin Menu."); //5
            logFlowPoint(String.format("Launch '%s'", entName)); //6

            // 1
            final WebDriver driver = startClientObject(context, client);
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);

            // 2
            NavigationSidebar navBar = new NavigationSidebar(driver);
            GameLibrary gameLibrary = navBar.gotoGameLibrary();
            logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

            // 3
            GameTile gameTile = new GameTile(driver, entOfferID);
            gameTile.waitForDownloadable();
            logPassFail(MacroGameLibrary.downloadFullEntitlement(driver, entOfferID), true);

            // 4
            boolean activationAdded = entitlement.addActivationFile(client);
            gameTile.play();
            boolean peggleLaunched = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, entProcess), 10000, 2000, 0);
            boolean activationUILaunched = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, "ActivationUI"), 5000, 700, 4000);
            logPassFail(activationAdded && peggleLaunched && !activationUILaunched, true);
            ProcessUtil.killProcess(client, entProcess);

            // 5
            MainMenu mainMenu = new MainMenu(driver);
            mainMenu.selectGoOffline();
            sleep(1000); // wait for Origin menu to update
            logPassFail(mainMenu.verifyItemEnabledInOrigin("Go Online"), true);

            // 6
            gameTile.play();
            peggleLaunched = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, entProcess), 10000, 2000, 0);
            activationUILaunched = Waits.pollingWaitEx(() -> ProcessUtil.isProcessRunning(client, "ActivationUI"), 5000, 700, 4000);
            logPassFail(peggleLaunched && !activationUILaunched, true);
            ProcessUtil.killProcess(client, entProcess);

            softAssertAll();

    }
}
