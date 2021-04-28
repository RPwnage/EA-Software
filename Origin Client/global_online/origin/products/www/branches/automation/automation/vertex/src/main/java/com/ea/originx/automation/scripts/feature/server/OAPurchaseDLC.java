package com.ea.originx.automation.scripts.feature.server;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DlcUpSellSuggestionDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.SlideoutExtraContent;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.Sims4;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OSInfo;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests if expansion for base game can be purchasable and downloadable
 *
 * @author rchoi
 */
public class OAPurchaseDLC extends EAXVxTestTemplate {

    @TestRail(caseId = 1016724)
    @Test(groups = {"client_only", "server", "services_smoke", "checkout", "int_only", "release_smoke"})
    public void testPurchaseDLC(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String environment = OSInfo.getEnvironment();
       
        final EntitlementInfo basegame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String basegameOfferID = basegame.getOfferId();
        final String basegameName = basegame.getName();
        final String dlcOfferID = Sims4.SIMS4_CITY_LIVING_NAME_OFFER_ID;
        final String dlcOfferName = Sims4.SIMS4_CITY_LIVING_NAME;

        logFlowPoint("Launch Origin and register as a new user");  //1
        logFlowPoint("Navigate to GDP Page and purchase entitlement which has a dlc"); //2
        logFlowPoint("Navigate to game library and check if purchased game appears in game library page"); //3
        logFlowPoint("Start downloading base game"); //4
        logFlowPoint("Base game is completely downloaded and installed"); //5
        logFlowPoint("Open slide out for base game and purchase dlc from expansion packs tab"); //6
        logFlowPoint("Verify DLC finishes downloading in Download Manager"); //7
        logFlowPoint("Verify dlc is successfully downloaded and installed in Game slide out"); //8

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        if(environment.equalsIgnoreCase("production")) {
            logPassFail(MacroPurchase.purchaseThroughPaypalOffCode(driver, basegame, userAccount.getEmail()), true);
        } else {
            logPassFail(MacroPurchase.purchaseEntitlement(driver, basegame), true);
        }

        // 3
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.isGameTileVisibleByName(basegameName), true);

        // 4
        logPassFail(MacroGameLibrary.startDownloadingEntitlement(driver, basegameOfferID), true);

        // 5
        logPassFail(MacroGameLibrary.finishDownloadingEntitlement(driver, basegameOfferID), true);

        //6
        //Need to maximize window for "Expansion Packs" tab in game slide out to be visible
        new MainMenu(driver).clickMaximizeButton();
        GameTile basegameTile = new GameTile(driver, basegameOfferID);
        GameSlideout basegameSlideout = basegameTile.openGameSlideout();
        basegameSlideout.waitForSlideout();
        sleep(7000); // wait for extra content tab to load
        basegameSlideout.clickExpansionPacksTab();
        SlideoutExtraContent extraContent = new SlideoutExtraContent(driver);
        sleep(7000); // wait for extra content to load
        extraContent.clickExtraContentButton(dlcOfferID);
        sleep(5000); //wait for dialog to load

        DlcUpSellSuggestionDialog dlcUpSellSuggestionDialog = new DlcUpSellSuggestionDialog(driver, dlcOfferID);
        dlcUpSellSuggestionDialog.clickContinueWithoutAddingDlcButton();

        if(environment.equalsIgnoreCase("production")) {
            logPassFail(MacroPurchase.handlePaymentInfoPageForProdPurchases(driver, dlcOfferName, userAccount.getEmail()), true);
        } else {
            logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);
        }

        // 7
        DownloadQueueTile dlcQueueTile = new DownloadQueueTile(driver,dlcOfferID);
        logPassFail(dlcQueueTile.waitForGameDownloadToComplete(), false);

        // 8
        logPassFail(extraContent.verifyExtraContentIsInstalled(dlcOfferID), false);

        softAssertAll();
    }
}