package com.ea.originx.automation.scripts.tools;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.UpdateAvailableDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.ExtraContentGameCard;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.SlideoutExtraContent;
import com.ea.originx.automation.lib.resources.games.GameFamilyInfo;
import com.ea.originx.automation.lib.resources.games.GameInfo;
import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;

import java.util.List;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;

/**
 * Test DLC Play Button for a game family e.g. SIMS3
 *
 * @author palui
 */
public class OADLCPlayButton extends EAXVxTestTemplate {

    GameFamilyInfo gameFamilyInfo = null;
    EntitlementInfo entitlement = null;
    List<GameInfo> dlcs = null;
    OriginClient client = null;

    public void testDLCPlayButton(ITestContext context, GameFamilyType familyType) throws Exception {

        client = OriginClientFactory.createPreserveGames(context);

        gameFamilyInfo = GameFamilyInfo.getGameFamilyInfo(familyType);
        entitlement = gameFamilyInfo.getBaseGameInfo().getEntitlementInfo();
        dlcs = gameFamilyInfo.getDlcInfos();

        final String entitlementName = entitlement.getName();

        final UserAccount userAccount;

        //Certain entitlements cannot be on the same account, so alternate accounts need to be used
        switch (familyType) {
            case THE_WITCHER_3:
                userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PERSONALIZATION_SPECIAL_ENTITLEMENTS);
                break;
            default:
                userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PERSONALIZATION_FULL_ENTITLEMENTS);
                break;
        }

        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as entitled user '%s' that has installed the game '%s' and all its DLCs",
                username, entitlementName));    //1
        logFlowPoint(String.format("Navigate to Game Library and ", entitlementName));  //2
        logFlowPoint(String.format("Verify account has base game installed"));  //3
        logFlowPoint(String.format("Locate '%s''s playable game tile", entitlementName));   //4
        logFlowPoint(String.format("Open Game Slideout for '%s' and navigate to the 'Extra Content' section", entitlementName));    //5

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as entitled user: " + username);
        } else {
            logFailExit("Failed: Cannot login as entitled user: " + username);
        }

        //2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Game library successfully navigated to");
        } else {
            logFailExit("Game library could not be reached");
        }

        //3
        GameInfo installedGame = null;
        for (GameInfo baseGame : gameFamilyInfo.getBaseGames()) {
            if (gameLibrary.isGameTileVisibleByOfferID(baseGame.getOfferId())) {
                installedGame = baseGame;
                break;
            }
        }

        if (installedGame != null) {
            logPass(String.format("Verified account has a base game %s installed.", installedGame));
        } else {
            logFailExit("Could not find base game for current family");
        }

        //4
        GameTile gameTile = new GameTile(driver, installedGame.getOfferId());
        if (gameTile.waitForReadyToPlay()) {
            logPass(String.format("Verified successful navigation to Game Library with playable '%s' game tile", installedGame));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or locate playable '%s' game tile", installedGame));
        }

        //5
        gameTile.showGameDetails();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        gameSlideout.clickExtraContentTab();
        SlideoutExtraContent extraContent = new SlideoutExtraContent(driver);
        boolean slideoutOpened = gameSlideout.verifyGameTitle(installedGame.getName());
        boolean extraContentOpened = extraContent.verifyBaseGameExtraContent(installedGame.getOfferId());
        if (slideoutOpened && extraContentOpened) {
            logPass("Verified Game Slideout 'Extra Content' section opens for " + installedGame);
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Slideout or open 'Extra Content' section for '%s'", installedGame));
        }

        EntitlementInfo dlc;
        String dlcName;
        String dlcOfferId;
        int dlcCount = 0;
        for (GameInfo dlcGameInfo : dlcs) {
            dlcCount++;
            dlc = GameInfo.getEntitlementInfoFor(dlcGameInfo);
            dlcName = dlc.getName();
            dlcOfferId = dlc.getOfferId();
            String dlcProcessName = dlc.getProcessName();
            final ExtraContentGameCard expansionCard = new ExtraContentGameCard(driver, dlcOfferId);

            logFlowPoint("Verify Play CTA appears for DLC");    //6
            logFlowPoint(String.format("Select the Play CTA on each DLC for %s and verify executable opens", entitlementName));   //7

            //6
            if (expansionCard.verifyGameCardVisible() && expansionCard.verifyPlayButtonVisible()) {
                logPass(String.format("Verified DLC %s '%s' game card is visible and has the 'Play' button", dlcCount, dlcName));
            } else {
                logFail(String.format("Failed: DLC %s '%s' game card is not visible or does not have the 'Play' button", dlcCount, dlcName));
                continue;
            }

            //7
            expansionCard.clickPlayButton();
            UpdateAvailableDialog updateAvailableDialog = new UpdateAvailableDialog(driver);
            if (updateAvailableDialog.waitIsVisible()) {
                updateAvailableDialog.clickPlayWithoutUpdatingButton();
            }

            if (dlc.isLaunched(client)) {
                logPass(String.format("Verified clicking DLC %s '%s' 'Play' button launches '%s'", dlcCount, dlcName, dlcProcessName));
            } else {
                logFailExit(String.format("Failed: Clicking DLC %s '%s' 'Play' button does not launch '%s'", dlcCount, dlcName, dlcProcessName));
            }

            //Maybe possible to add the polling wait to entitlementinfo
            ProcessUtil.killProcess(client, dlcProcessName);
            if (Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, dlcProcessName))) {
                logPass(String.format("Verified proccess %s successfully closed", dlcProcessName));
            } else {
                logFailExit(String.format("Could not kill proccess %s", dlcProcessName));
            }

            //Witcher 3 process does not close fast enough but Origin allows Play CTA to be clickable. Added sleep to make sure process is fully closed.
            if (dlcProcessName.equals("witcher3.exe")) {
                Waits.sleep(10000);
            }

        }
        softAssertAll();
    }

    /**
     * Override testCleanup to kill all games launched
     *
     * @param context (@link ITestContext) context for the running test
     */
    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (client != null) {
            if (entitlement != null) {
                entitlement.killLaunchedProcess(client);
            }
            for (GameInfo dlc : dlcs) {
                ProcessUtil.killProcess(client, dlc.getProcessName());
            }
        }

        super.testCleanUp(result, context);
    }

}
