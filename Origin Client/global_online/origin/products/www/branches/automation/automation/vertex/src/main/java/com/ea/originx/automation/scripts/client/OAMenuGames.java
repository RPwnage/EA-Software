package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.originaccess.VaultPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.Notepad;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 *  Tests the items present in Games Menu
 *
 * @author cbalea
 */
public class OAMenuGames extends EAXVxTestTemplate{
    
    @TestRail(caseId = 9760)
    @Test(groups = {"client", "client_only", "full_regression"})
    public void testMenuGames(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        Notepad nonOriginGame = new Notepad();
        String nonOriginGameName = nonOriginGame.getName();
        
        logFlowPoint("Launch and log into Origin with a subscriber account"); // 1
        logFlowPoint("Verify 'Add Origin Access Vault game' is in the drop down menu and clicking it brings user to 'Origin Access Vault' page"); // 2
        logFlowPoint("Verify 'Add Non-Origin Game' is in the drop down menu and clicking it triggers a file location process"); // 3
        logFlowPoint("Verify 'Reload Game Library' is in the drop down menu and clicking it refreshes the game library  "); // 4

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + userAccount.getUsername());
        }
        
        //2
        MainMenu mainMenu = new MainMenu(driver);
        boolean addOriginGame = mainMenu.verifyItemEnabledInGames("Add Origin Access Vault game");
        mainMenu.selectAddVaultGame();
        boolean originVault = new VaultPage(driver).verifyPageReached();
        if(addOriginGame && originVault){
            logPass("Verified 'Add Origin Access Vault game' is in the 'Games' drop down and clicking it bring user to 'Origin Access Vault' page");
        } else {
            logFailExit("Failed to verify 'Add origin Access game' is in the 'Games' drop down or page 'Origin Access Vault' hasn't been reached");
        }
        
        //3
        boolean addNonOriginGame = mainMenu.verifyItemEnabledInGames("Add Non-Origin Game\u2026");
        mainMenu.selectAddNonOriginGameSubscriber();
        sleep(5000); // Waiting for dialog to appear and focus
        new RobotKeyboard().typeThenHitEnter(client, nonOriginGame.getExecutablePath(client));
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, gameLibrary.getGameTileOfferIdByName(nonOriginGameName));
        if (gameTile.isGameTileVisible() && addNonOriginGame) {
            logPass("Verified 'Add Non-Origin Game...' is in Games drop down and clicking it triggers a file location process");
        } else {
            logFailExit("Failed to verify 'Add Non-Origin Game...' in Games drop down or the file location process did not trigger");
        }
        
        //4
        boolean reloadLibrary = mainMenu.verifyItemEnabledInGames("Reload Game Library");
        mainMenu.selectReloadMyGamesSubscriber();
        if(reloadLibrary){
            logPass("Verified 'Reload Game Library' is in the 'Games' drop down");
        } else {
            logFailExit("Failed to verify 'Reload Game Library' in the 'Games' drop down");
        }
        
        softAssertAll();
    }
}
