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
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that DLC purchases are downloadable.
 *
 * @author esdecastro
 * */
public class OAPurchaseDownloadDLC extends EAXVxTestTemplate{

    @TestRail(caseId = 9950)
    @Test(groups = {"gamedownloader", "client_only", "full_regression"})
    public void testPurchaseDownloadDLC(ITestContext context) throws Exception{

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();
        EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        final String dlcOfferID = Battlefield4.BF4_LEGACY_OPERATIONS_OFFER_ID;
        final String baseGameOfferID = baseGame.getOfferId();

        logFlowPoint("Open Origin and log in to an account that owns a base game that has a free DLC"); // 1
        logFlowPoint("Navigate to the 'Game Library'"); // 2
        logFlowPoint("Click on the base game's and view the Free DLC's game tile in the OGD's Extra content tab"); // 3
        logFlowPoint("Entitle the free DLC"); //4
        logFlowPoint("Verify that the Download button appears on the free DLC tile"); // 5
        logFlowPoint("Click the 'Download' button and verify that the base game begins download " +
                "and the DLC is added to the queue"); // 6

        WebDriver driver = startClientObject(context, client);

        // 1
        boolean isLoggedIn = MacroLogin.startLogin(driver, userAccount);
        boolean isBaseGamePurchased = MacroPurchase.purchaseEntitlement(driver, baseGame);
        logPassFail(isLoggedIn && isBaseGamePurchased, true);

        // 2
        logPassFail(new NavigationSidebar(driver).gotoGameLibrary().verifyGameLibraryPageReached(), true);

        // 3
        new MainMenu(driver).clickMaximizeButton();
        new GameTile(driver, baseGameOfferID).openGameSlideout().clickExtraContentTab();
        ExtraContentGameCard dlcGameCard = new ExtraContentGameCard(driver, dlcOfferID);
        dlcGameCard.scrollToGameCard();
        logPassFail(dlcGameCard.verifyGameCardVisible(), true);

        // 4
        dlcGameCard.clickBuyButton();
        sleep(5500); // Wait added to let the Thank You Modal close and open up again. Bug filed with a maintenance ticket to remove this once the bug is fixed.
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        logPassFail(checkoutConfirmation.waitIsVisible(), true);

        //5
        checkoutConfirmation.close();
        logPassFail(dlcGameCard.verifyDownloadButtonVisible(), true);

        // 5
        dlcGameCard.scrollToGameCard();
        dlcGameCard.clickDownloadButton();
        MacroGameLibrary.handleDialogs(driver, baseGameOfferID, new DownloadOptions().setUncheckExtraContent(false));
        DownloadQueueTile baseGameQueueTile = new DownloadQueueTile(driver, baseGameOfferID);
        DownloadQueueTile dlcQueueTile = new DownloadQueueTile(driver,dlcOfferID);
        logPassFail(baseGameQueueTile.isGameDownloading() && dlcQueueTile.isGameQueued(), true);

        softAssertAll();
    }
}
