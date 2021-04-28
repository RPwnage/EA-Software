package com.ea.originx.automation.scripts.myhome;

import com.ea.originx.automation.lib.macroaction.MacroCommon;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.RemoveGameMsgBox;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.originclient.account.UserAccount;
import org.openqa.selenium.WebDriver;

/**
 * Tests the 'My Home' empty game state.
 *
 * @author caleung
 */

public class OAMyHomeEmptyGameState extends EAXVxTestTemplate {

    @TestRail(caseId = 1936023)
    @Test(groups = {"myhome", "client_only", "full_regression", "release_smoke"})
    public void testMyHomeEmptyGameState(ITestContext context) throws Exception {

        OriginClient client = OriginClientFactory.create(context);
        final UserAccount user = AccountManager.getInstance().createNewThrowAwayAccount();
        
        final String EXE_FOLDER_PATH = "C:\\Program Files\\Internet Explorer\\iexplore.exe";

        logFlowPoint("Log into Origin with a newly created account.."); // 1
        logFlowPoint("Select 'Add Non-Origin Game' from the 'Games' menu, choose an executable that Origin accepts, and verify Origin redirects user to the 'Game Library'."); // 2
        logFlowPoint("Go to 'My Home' and verify that it is now populated with articles and news."); // 3
        logFlowPoint("Go to the 'Game Library' and remove the non-Origin game from the 'Game Library'."); // 4
        logFlowPoint("Go to 'My Home', verify it is back to the empty game state, and verify the 'Browse Games' option appears."); // 5
        logFlowPoint("Verify the 'Buy Some Games' option appears."); // 6
        logFlowPoint("Verify the 'Check Out Origin Access' option appears."); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        // 2
        MacroCommon.addNonOriginGame(EXE_FOLDER_PATH, driver, client);
        GameLibrary gameLibrary = new GameLibrary(driver);
        logPassFail(Waits.pollingWait(() -> gameLibrary.verifyGameLibraryPageReached()), true);

        // 3
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.gotoDiscoverPage();
        DiscoverPage discoverPage = new DiscoverPage(driver);
        logPassFail(discoverPage.verifyGameUpdatesAndEventsSectionVisible(), false);

        // 4
        navigationSidebar.gotoGameLibrary();
        String offerId = gameLibrary.getGameTileOfferIds().get(0);
        new GameTile(driver, offerId).removeFromLibrary();
        new RemoveGameMsgBox(driver).clickRemoveButton();
        logPassFail(gameLibrary.verifyGameLibraryPageEmpty(), true);

        // 5
        navigationSidebar.gotoDiscoverPage();
        boolean isEmptyGameState = discoverPage.verifyEmptyGameState();
        boolean isPlayFreeGameTileVisible = discoverPage.verifyBrowseGamesTileVisible();
        logPassFail(isEmptyGameState && isPlayFreeGameTileVisible, false);

        // 6
        logPassFail(discoverPage.verifyBuySomeGamesTileVisible(), false);

        // 7
        logPassFail(discoverPage.verifyCheckOutOATileVisible(), false);

        softAssertAll();
    }
}