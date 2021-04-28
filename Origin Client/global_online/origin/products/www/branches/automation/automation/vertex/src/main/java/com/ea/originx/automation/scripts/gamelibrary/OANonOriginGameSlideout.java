package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideoutCogwheel;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.RemoveGameMsgBox;
import com.ea.originx.automation.lib.resources.games.Notepad;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test non-origin game slideout
 * (This ticket should be combine with OANonOriginGameLaunchPath - C11129)
 *
 * @author palui
 */
public class OANonOriginGameSlideout extends EAXVxTestTemplate {

    @TestRail(caseId = 11129)
    @Test(groups = {"gamelibrary", "client_only", "full_regression"})
    public void testNonOriginGameSlideout(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        Notepad nonOriginGame = new Notepad();
        String nonOriginGameName = nonOriginGame.getName();
        String nonOriginGameOfferId = nonOriginGame.getOfferId();

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as newly registered user: " + username);//1
        logFlowPoint(String.format("Select 'Add Non-Origin Games' at 'Games' menu to add '%s'. Verify '%s' is added to the Game Library",
                nonOriginGameName, nonOriginGameName)); //2
        logFlowPoint(String.format("Verify image for the '%s' game tile is visible", nonOriginGameName)); //3
        logFlowPoint(String.format("Open the 'Game Slideout' and verify the name is '%s'", nonOriginGameName));//4
        logFlowPoint(String.format("Verify '%s' game slideout has expected game background image visible", nonOriginGameName));//5
        logFlowPoint(String.format("Verify '%s' game slideout has a 'Non-Origin Game' label", nonOriginGameName));//6
        logFlowPoint(String.format("Launch '%s' from the game slideout 'Play' button", nonOriginGameName));//7
        logFlowPoint(String.format("Kill '%s'", nonOriginGameName));//8
        logFlowPoint(String.format("Click the favorites button at the game slideout. Verify '%s' is added to the favorites list in the Game Library", nonOriginGameName));//9
        logFlowPoint(String.format("Click the favorites button again. Verify '%s' is removed from the favorites list", nonOriginGameName));//10
        logFlowPoint(String.format("Verify '%s' game slideout has no navigation links", nonOriginGameName));//11
        logFlowPoint("Click 'Remove from library' at the cogwheel context menu. Verify 'Remove Game' message box appears");//12
        logFlowPoint(String.format("Click 'Cancel' at the 'Remove Game' message box. Verify '%s' remians in the Game Library", nonOriginGameName));//13
        logFlowPoint("Click 'Remove from library' again at the cogwheel context menu. Verify 'Remove Game' message box re-appears");//14
        logFlowPoint(String.format("Click 'Remove' at the 'Remove Game' message box. Verify '%s' is removed from the Game Library", nonOriginGameName));//15

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + username);
        } else {
            logFailExit("Failed: Cannot login as user: " + username);
        }

        //2
        new MainMenu(driver).selectAddNonOriginGame();
        new RobotKeyboard().typeThenHitEnter(client, nonOriginGame.getExecutablePath(client));
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, nonOriginGameOfferId);
        if (Waits.pollingWait(gameTile::isGameTileVisible)) {
            logPass(String.format("Verified '%s' successfully added to the Game Library", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' is not shown", nonOriginGameName));
        }

        //3
        if (gameTile.isGameTileDataImageVisible()) {
            logPass(String.format("Verified image for '%s' game tile is visible", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: image for '%s' game tile is not found or is not visible", nonOriginGameName));
        }

        //4
        gameTile.showGameDetails();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        if (Waits.pollingWait(() -> gameSlideout.verifyGameTitle(nonOriginGameName))) {
            logPass(String.format("Verified Game Slideout opens with matching name for '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot open Game Slideout with matching name for '%s'", nonOriginGameName));
        }

        //5
        boolean expectedImageShown = gameSlideout.verifyBackgroundImageVisible()
                && gameSlideout.getBackgroundImageSrc().equals(gameTile.getImageSrc());
        if (expectedImageShown) {
            logPass(String.format("Verified Game Slideout for '%s' has expected background image visible", nonOriginGameName));
        } else {
            logFail(String.format("Failed: Game Slideout for '%s' does not have expected background image visible", nonOriginGameName));
        }

        //6
        if (gameSlideout.verifyNonOriginGameLabel()) {
            logPass(String.format("Verified Game Slideout indicates '%s' as 'Non-Origin Game'", nonOriginGameName));
        } else {
            logFail(String.format("Failed: Game Slideout does not indicate '%s' as 'Non-Origin Game'", nonOriginGameName));
        }

        //7
        gameSlideout.startPlay(1000);
        if (Waits.pollingWaitEx(() -> nonOriginGame.isLaunched(client))) {
            logPass(String.format("Verified successful launch of '%s' using the 'Play' button", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot launch '%s' using the 'Play' button", nonOriginGameName));
        }

        //8
        if (nonOriginGame.killLaunchedProcess(client)) {
            logPass(String.format("Verified successful termination of '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot terminate '%s'", nonOriginGameName));
        }

        //9
        gameSlideout.clickFavoritesButton();
        if (Waits.pollingWait(() -> gameLibrary.isFavoritesGameTile(nonOriginGameOfferId))) {
            logPass(String.format("Verified '%s' is added to the favorites list of the Game Library", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: '%s' is not added to the favorites list of the Game Library", nonOriginGameName));
        }

        //10
        gameSlideout.clickFavoritesButton();
        if (Waits.pollingWait(() -> !gameLibrary.isFavoritesGameTile(nonOriginGameOfferId))) {
            logPass(String.format("Verified '%s' is removed from the favorites list", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: '%s' is not removed from the favorites list", nonOriginGameName));
        }

        //11
        boolean anyNavLinksVisible = gameSlideout.verifyNavLinksVisible()
                || gameSlideout.verifyFriendsWhoPlayNavLinkVisible()
                || gameSlideout.verifyAchievementsNavLinkVisible()
                || gameSlideout.verifyExtraContentNavLinkVisible();
        if (!anyNavLinksVisible) {
            logPass("Verified no navigation links are shown for Non-Origin Games");
        } else {
            logFail("Failed: Some navigation links are shown for Non-Origin Games");
        }

        //12
        GameSlideoutCogwheel cogwheel = gameSlideout.getCogwheel();
        cogwheel.removeFromLibrary();
        final RemoveGameMsgBox removeGameMsgBox = new RemoveGameMsgBox(driver);
        if (Waits.pollingWait(() -> removeGameMsgBox.verifyVisible(nonOriginGameName))) {
            logPass(String.format("Verified 'Remove Game' message box appears with the expected titles and message content for '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: 'Remove Game' message box does not appear or has unexpected titles and message content for '%s'", nonOriginGameName));
        }

        //13
        removeGameMsgBox.clickCancelButton();
        if (gameTile.isGameTileVisible()) {
            logPass(String.format("Verified '%s' remains in the Game Library", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: '%s' is removed from Game Library", nonOriginGameName));
        }

        //14
        cogwheel.removeFromLibrary();
        final RemoveGameMsgBox removeGameMsgBox2 = new RemoveGameMsgBox(driver); // polling wait requires final-ish variable
        if (Waits.pollingWait(() -> removeGameMsgBox2.verifyVisible(nonOriginGameName))) {
            logPass(String.format("Verified 'Remove Game' message box re-appears with the expected titles and message content for '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: 'Remove Game' message box does not re-appear or has unexpected titles and message content for '%s'", nonOriginGameName));
        }

        //15
        removeGameMsgBox2.clickRemoveButton();
        boolean gameRemoved = Waits.pollingWait(() -> !gameTile.isGameTileVisible());
        if (gameRemoved) {
            logPass(String.format("Verified '%s' is removed from the Game Library", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: '%s' remains in the Game Library", nonOriginGameName));
        }

        softAssertAll();
    }
}
