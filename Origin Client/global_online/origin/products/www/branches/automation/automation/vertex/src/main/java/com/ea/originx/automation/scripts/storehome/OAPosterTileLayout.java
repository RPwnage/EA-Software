package com.ea.originx.automation.scripts.storehome;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreGameTile;
import com.ea.originx.automation.lib.pageobjects.store.StoreHomeTile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests that some basic store tile information is displayed
 *
 * @author lscholte
 */
public class OAPosterTileLayout extends EAXVxTestTemplate {

    @TestRail(caseId = 12079)
    @Test(groups = {"storehome","full_regression"})
    public void testPosterTileLayout(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount user = AccountManager.getRandomAccount();

        logFlowPoint("Log into Origin with " + user.getUsername()); //1
        logFlowPoint("Navigate to the Browse Games page"); //2
        logFlowPoint("Verify all store tiles' art pack is displayed"); //3
        logFlowPoint("Verify all store tiles' title is displayed"); //4

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        BrowseGamesPage browseGamesPage = navBar.gotoBrowseGamesPage();
        logPassFail(browseGamesPage.verifyBrowseGamesPageReached(), true);

        List<String> failedTitle = new ArrayList<>();
        List<String> failedPackArt = new ArrayList<>();
        for (StoreHomeTile storeTile : browseGamesPage.getLoadedStoreGameTiles()) {
            final String productTitle = storeTile.getTileTitle();
            if (!storeTile.verifyTileTitleVisible()) {
                failedTitle.add(productTitle);
            }
            if (!storeTile.verifyTileImageVisible()) {
                failedPackArt.add(productTitle);
            }
        }

        //3
        logPassFail(failedPackArt.isEmpty(), false);

        //4
        logPassFail(failedTitle.isEmpty(), false);

        softAssertAll();
    }
}
