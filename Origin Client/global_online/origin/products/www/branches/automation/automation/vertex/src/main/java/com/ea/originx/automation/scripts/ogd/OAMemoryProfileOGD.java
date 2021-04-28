package com.ea.originx.automation.scripts.ogd;

import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests memory profile of the OGD in order to expose memory leaks
 *
 * @author mdobre
 */
public class OAMemoryProfileOGD extends EAXVxTestTemplate {

    @Test(groups = {"ogd", "memory_profile"})
    public void testPerformanceOGD(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        OADipSmallGame entitlement = new OADipSmallGame();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        userAccount.cleanFriends();
        final UserAccount firstFriendAccount = AccountManager.getRandomAccount();
        final UserAccount secondFriendAccount = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(userAccount, firstFriendAccount, secondFriendAccount);

        logFlowPoint("Login to Origin with an existing account."); //1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Open the OGD, close the OGD and perform garbage collection."); //2-16
        }

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + userAccount.getUsername());
        }

        //2-16
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = new GameLibrary(driver);
        GameTile gameTile = new GameTile(driver, entitlement.getOfferId());
        GameSlideout gameSlideout = null;
        boolean isOGDOpen = false;
        boolean isOGDClosed = false;
        boolean isGarbageCollectionSuccesful = false;
        
        for (int i = 0; i < 15; i++) {
            navigationSidebar.gotoGameLibrary();
            gameLibrary.waitForPage();
            gameSlideout = gameTile.openGameSlideout();
            
            //Open the OGD
            isOGDOpen = gameSlideout.waitForSlideout();
            
            //Close the OGD
            gameSlideout.clickSlideoutCloseButton();
            isOGDClosed = !gameSlideout.waitForSlideout();
            
            //Perform Garbage Collection
            isGarbageCollectionSuccesful = garbageCollect(driver, context);
            
            if (isOGDOpen && isOGDClosed && isGarbageCollectionSuccesful) {
                logPass("Successfully opened the OGD, closed the OGD and performed garbage collection.");
            } else {
                logFail("Failed to open the OGD, close the OGD or perform garbage collection.");
            }
        }
        softAssertAll();
    }
}
