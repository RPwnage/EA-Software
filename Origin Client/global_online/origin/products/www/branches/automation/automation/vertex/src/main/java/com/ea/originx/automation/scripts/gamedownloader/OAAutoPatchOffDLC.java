package com.ea.originx.automation.scripts.gamedownloader;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.DownloadQueueTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.ExtraContentGameCard;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

public class OAAutoPatchOffDLC extends EAXVxTestTemplate {

    @TestRail(caseId = 9950)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testAutoPatchOff(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String dlcOfferID = Battlefield4.BF4_LEGACY_OPERATIONS_OFFER_ID;
        final String baseGameOfferID = baseGame.getOfferId();

        logFlowPoint("Open Origin and log in to an account that owns a base game that has a free DLC"); // 1
        logFlowPoint("Navigate to 'Application Settings' "); // 2
        logFlowPoint("Turn off 'Automatic game updates' in 'Application Settings' "); // 3
        logFlowPoint("Navigate to the 'Game Library' and Click on the base game's and view the Free DLC's game tile in the OGD's 'Extra content' tab"); // 4
        logFlowPoint("Entitle the free DLC"); //5
        logFlowPoint("After entitling a DLC, Verify that the 'Download' button appears on the free DLC tile"); // 6
        logFlowPoint("Click the 'Download' button and verify that the base game begins download and the DLC is added to the queue even when 'Automatic game updates' are turned off"); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        boolean isLoggedIn = MacroLogin.startLogin(driver, userAccount);
        boolean isBaseGamePurchased = MacroPurchase.purchaseEntitlement(driver, baseGame);
        logPassFail(isLoggedIn && isBaseGamePurchased, true);

        // 2
        AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
        logPassFail(Waits.pollingWait(appSettings::verifyAppSettingsReached), true);

        //3
        appSettings.setKeepGamesUpToDate(false);
        logPassFail(appSettings.verifyKeepGamesUpToDate(false), true);

        // 4
        new NavigationSidebar(driver).gotoGameLibrary();
        new MainMenu(driver).clickMaximizeButton();
        new GameTile(driver, baseGameOfferID).openGameSlideout().clickExtraContentTab();
        ExtraContentGameCard dlcGameCard = new ExtraContentGameCard(driver, dlcOfferID);
        dlcGameCard.scrollToGameCard();
        logPassFail(dlcGameCard.verifyGameCardVisible(), true);

        // 5
        dlcGameCard.clickBuyButton();
        sleep(5500); // Wait added to let the Thank You Modal close and open up again. Bug filed with a maintenance ticket to remove this once the bug is fixed.
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);

        // 6
        checkoutConfirmation.close();
        logPassFail(dlcGameCard.verifyDownloadButtonVisible(), true);

        // 7
        dlcGameCard.scrollToGameCard();
        dlcGameCard.clickDownloadButton();
        MacroGameLibrary.handleDialogs(driver, baseGameOfferID, new DownloadOptions().setUncheckExtraContent(false));
        DownloadQueueTile baseGameQueueTile = new DownloadQueueTile(driver, baseGameOfferID);
        DownloadQueueTile dlcQueueTile = new DownloadQueueTile(driver,dlcOfferID);
        logPassFail(baseGameQueueTile.isGameDownloading() && dlcQueueTile.isGameQueued(), true);

        softAssertAll();
    }
}
