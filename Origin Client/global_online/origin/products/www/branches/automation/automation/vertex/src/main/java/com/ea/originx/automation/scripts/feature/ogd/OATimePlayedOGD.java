package com.ea.originx.automation.scripts.feature.ogd;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author cvanichsarn
 */
public class OATimePlayedOGD extends EAXVxTestTemplate {

    @TestRail(caseId = 3103324)
    @Test(groups = {"ogd_smoke", "ogd", "allfeaturesmoke"})
    public void testTimePlayedOGD(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        OADipSmallGame entitlement = new OADipSmallGame();
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String accountName = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as " + accountName); //1
        logFlowPoint("Navigate to the Game Library"); //2
        logFlowPoint("Download and Install the DiP Small offer"); //3
        logFlowPoint("Launch DiP Small offer"); //4
        logFlowPoint("Exit DiP Small offer"); //5
        logFlowPoint("Open the OGD for DiP Small"); //6
        logFlowPoint("Verify that there is text in the 'Hours Played' section of the OGD"); //6

        //1
        final WebDriver driverUser = startClientObject(context, client);
        if (MacroLogin.startLogin(driverUser, userAccount)) {
            logPass("Verified login successful to user account: " + accountName);
        } else {
            logFailExit("Failed: Cannot login to user account: " + accountName);
        }

        //2
        new NavigationSidebar(driverUser).gotoGameLibrary();
        GameTile gameTile = new GameTile(driverUser, entitlementOfferId);
        if (gameTile.waitForDownloadable()) {
            logPass("Verified user successfully navigated to 'Game Library' with " + entitlementName + "'s game tile");
        } else {
            logFailExit("Failed: User cannot navigate to 'Game Library' or locate " + entitlementName + "'s game tile");
        }

        //3
        boolean downloaded = MacroGameLibrary.downloadFullEntitlement(driverUser, entitlementOfferId);
        if (downloaded) {
            logPass("Verified user succesfully downloaded " + entitlementName);
        } else {
            logFailExit("Failed: User was unable to download " + entitlementName);
        }

        //4
        gameTile.play();
        if (Waits.pollingWaitEx(() -> entitlement.isLaunched(client))) {
            logPass("Verified user successfully launched " + entitlementName);
        } else {
            logFailExit("Failed: User was unable to launch " + entitlementName);
        }

        //5
        entitlement.killLaunchedProcess(client);
        if (Waits.pollingWaitEx(() -> entitlement.killLaunchedProcess(client))) {
            logPass("Verified " + entitlementName + " was closed");
        } else {
            logFailExit("Failed: " + entitlementName + " failed to be closed");
        }

        //6
        GameSlideout gameSlideout = gameTile.openGameSlideout();
        if (Waits.pollingWait(() -> gameSlideout.waitForSlideout())) {
            logPass("Verified the OGD for " + entitlementName + " opened");
        } else {
            logFailExit("Failed: OGD failed to open for " + entitlementName);
        }

        //7
        if (Waits.pollingWait(() -> gameSlideout.verifyActualHoursPlayedText())) {
            logPass("Verified the hours played for " + entitlementName + " is displayed");
        } else {
            logFailExit("Failed: The hours played for " + entitlementName + " is not displayed");
        }

        softAssertAll();
    }
}
