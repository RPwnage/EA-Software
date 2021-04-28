package com.ea.originx.automation.scripts.feature.gamelibrary;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.dialog.RemoveGameMsgBox;
import com.ea.originx.automation.lib.resources.games.Notepad;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test adding and removing non-origin game
 *
 * @author rocky
 */
public class OAAddLaunchNOG extends EAXVxTestTemplate {

    @TestRail(caseId = 3087351)
    @Test(groups = {"gamelibrary", "gamelibrary_smoke", "client_only", "allfeaturesmoke"})
    public void testAddLaunchNOG(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        Notepad nonOriginGame = new Notepad();
        String nonOriginGameName = nonOriginGame.getName();

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and login as newly registered user: " + username);//1
        logFlowPoint(String.format("Select 'Add Non-Origin Games' at 'Games' menu to add '%s' and verify '%s' is added to the Game Library",
                nonOriginGameName, nonOriginGameName));//2
        logFlowPoint(String.format("Launch '%s' from the mouse right click menu 'play' on the tile", nonOriginGameName));//3
        logFlowPoint(String.format("Kill '%s'", nonOriginGameName));//4
        logFlowPoint(String.format("Remove '%s' from the game library through mouse right click menu on the tile and verify it is removed from the Game Library", nonOriginGameName));//5

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new MainMenu(driver).selectAddNonOriginGame();
        new RobotKeyboard().typeThenHitEnter(client, nonOriginGame.getExecutablePath(client));
        GameLibrary gameLibrary = new GameLibrary(driver);
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, gameLibrary.getGameTileOfferIdByName(nonOriginGameName));
        logPassFail(Waits.pollingWait(() -> gameTile.isGameTileVisible()), true);

        //3
        gameTile.play();
        logPassFail(Waits.pollingWaitEx(() -> nonOriginGame.isLaunched(client)), true);

        //4
        logPassFail(nonOriginGame.killLaunchedProcess(client), true);

        //5
        gameTile.removeFromLibrary();
        new RemoveGameMsgBox(driver).clickRemoveButton();
        boolean gameRemoved = Waits.pollingWait(() -> !gameTile.isGameTileVisible());
        logPassFail(gameRemoved, true);

        softAssertAll();
    }
}
