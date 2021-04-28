package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.OriginAccessSettingsPage;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.OriginAccessService;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that an account with expired subscription loses its benefits
 * 
 * @author cbalea
 */
public class OAOriginAccessExpiredBenefits extends EAXVxTestTemplate {

    @TestRail(caseId = 3064022)
    @Test(groups = {"originaccess", "full_regression", "client_only"})
    public void ExpiredGameLibraryContent(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo vaultEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.Limbo);
        final String vaultEntitlementOfferID = vaultEntitlement.getOfferId();
        final EntitlementInfo purchaseEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BIG_MONEY);
        final String purchaseEntitlementOfferID = purchaseEntitlement.getOfferId();
        EntitlementInfo vaultEntitlementToAdd = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        logFlowPoint("Launch and log into 'Origin'"); // 1
        logFlowPoint("Click on the mini profile and select 'EA Account and Billing'"); // 2
        logFlowPoint("Click 'Origin Access' tab and verify the subscription status shows up as expired"); // 3
        logFlowPoint("Go back to 'Origin' and attempt to add a vault game to library"); // 4
        logFlowPoint("Go to 'Game Library' and verify user cannot play vault games"); // 5
        logFlowPoint("Click on an expired tile and verify there is no download link"); // 6
        logFlowPoint("Verify a purchased game can be launched"); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // setting preconditions
        MacroOriginAccess.purchaseOriginPremierAccess(driver);
        MacroGDP.loadGdpPage(driver, vaultEntitlement);
        MacroGDP.addVaultGameAndCheckViewInLibraryCTAPresent(driver);
        MacroPurchase.purchaseEntitlement(driver, purchaseEntitlement);

        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.gotoGameLibrary();
        MacroGameLibrary.downloadFullEntitlement(driver, vaultEntitlementOfferID);
        MacroGameLibrary.downloadFullEntitlement(driver, purchaseEntitlementOfferID);
        OriginAccessService.immediateCancelSubscription(userAccount);
        sleep(10000); // waiting for cancellation to take effect

        // 2
        WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        logPassFail(accountSettingsPage.verifyAccountSettingsPageReached(), true);

        // 3
        accountSettingsPage.gotoOriginAccessSection();
        logPassFail(new OriginAccessSettingsPage(browserDriver).isMembershipInactive(), true);

        // 4
        browserDriver.close();
        MacroGDP.loadGdpPage(driver, vaultEntitlementToAdd);
        logPassFail(!new GDPActionCTA(driver).verifyAddToLibraryVaultGameCTAVisible(), false);

        // 5
        navigationSidebar.gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, vaultEntitlementOfferID);
        logPassFail(!gameTile.isReadyToPlay(), false);

        // 6
        gameTile.openGameSlideout();
        GameSlideout gameSlideout = new GameSlideout(driver);
        logPassFail(!gameSlideout.verifyButtonVisible(), false);

        // 7
        gameSlideout.clickSlideoutCloseButton();
        logPassFail(new GameTile(driver, purchaseEntitlementOfferID).isReadyToPlay(), true);
    }
}
