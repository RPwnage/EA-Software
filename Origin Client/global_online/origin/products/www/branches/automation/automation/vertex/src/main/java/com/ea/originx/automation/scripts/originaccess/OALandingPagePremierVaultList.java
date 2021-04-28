package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.AccountService;
import java.util.List;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the 'Games in EA Access' displayed entitlements
 *
 * @author cbalea
 */
public class OALandingPagePremierVaultList extends EAXVxTestTemplate {

    @TestRail(caseId = 3246567)
    @Test(groups = {"originaccess", "full_regression"})
    public void testLandingPagePremierVaultList(ITestContext context) throws Exception {

        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount account = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        AccountService.addAccountToPremierWhitelist(account);

        logFlowPoint("Login into origin"); // 1
        logFlowPoint("Click 'Origin Access' tab from the left navigation menu and verify that the 'Origin Access' landing page is shown"); // 2
        logFlowPoint("Scroll down to the premier 'Vault List' section and verify there is a title for the section as'Premier  PC'"); // 3
        logFlowPoint("Verify that the horizontal tiles for the games included in section are displayed"); // 4
        logFlowPoint("Verify there is a 'Coming Soon' or 'Available-Now' violators displayed on the top left corner of the tiles"); // 5
        logFlowPoint("Verify there is game name displayed below the game tile"); // 6
        logFlowPoint("Verify there is a short description of the game below the game name"); // 7
        logFlowPoint("Verify clicking the game tile navigates to the GPD of the game"); // 8
        logFlowPoint("Verify clicking the game name navigates to the GDP of the game"); // 9

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, account), true);
        if (isClient) {
            new MainMenu(driver).clickMaximizeButton();
        }

        // 2
        new NavigationSidebar(driver).clickOriginAccessLink();
        OriginAccessPage originAccessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        logPassFail(originAccessPage.verifyPageReached(), true);

        // 3
        originAccessPage.clickGamesInEAAccessTab();
        logPassFail(originAccessPage.verifyGamesInEAAccessSection(), true);

        // 4
        logPassFail(originAccessPage.verifyGamesInEAAccessSectionTiles(), false);

        // 5
        logPassFail(originAccessPage.verifyGamesInEAAccessSectionTilesViolator(), false);

        // 6
        logPassFail(originAccessPage.verifyGamesInEAAccessSectionTilesTitle(), false);

        // 7
        logPassFail(originAccessPage.verifyGamesInEAAccessSectionTilesDescription(), false);

        // 8
        List<String> tileTitle = originAccessPage.getGamesInEAAccessSectionTilesTitle();
        originAccessPage.clickGamesInEAAccessSectionTile();
        GDPHeader gdpHeader = new GDPHeader(driver);
        logPassFail(gdpHeader.verifyGameTitle(tileTitle), true);

        // 9
        driver.navigate().back();
        originAccessPage.waitForPageToLoad();
        originAccessPage.clickGamesInEAAccessSectionTileTitle();
        logPassFail(gdpHeader.verifyGameTitle(tileTitle), true);

        softAssertAll();
    }
}
