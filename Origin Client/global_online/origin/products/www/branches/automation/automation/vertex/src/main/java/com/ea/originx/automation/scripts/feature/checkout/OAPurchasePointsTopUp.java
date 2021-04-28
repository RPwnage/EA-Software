package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.PurchasePointsPackageDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.SlideoutExtraContent;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.pageobjects.store.ThankYouPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.MassEffect3;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that a user can purchase an entitlement's dlc using 'Purchase Points'
 *
 * @author rchoi
 */
public class OAPurchasePointsTopUp extends EAXVxTestTemplate {

    @Test(groups = {"checkout_smoke", "checkout", "int_only", "client_only", "allfeaturesmoke"})
    public void testPurchasePointsTopUp(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final EntitlementInfo baseGame = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ME3);
        final String baseGameOfferID = baseGame.getOfferId();
        final String dlcOfferID = MassEffect3.ME3_CITADEL_OFFER_ID;

        logFlowPoint("Login to Origin with a newly registered user account");  //1
        logFlowPoint("Purchase the base game of an entitlement that uses 'Purchase Points' to purchase dlc"); //2
        logFlowPoint("Go to the 'Game Library' page"); //3
        logFlowPoint("Verify that the base game appears in the 'Game Library'"); //4
        logFlowPoint("Open the slideout for the base game"); //5
        logFlowPoint("Click the 'Extra Content' tab and verify that the dlc content exists by offer-id"); //6
        logFlowPoint("Verify that a dialog appears which directs the user to purchase a points package appears"); //7
        logFlowPoint("Click the 'Review Order' button and verify that the 'Payment Information' page appears for purchasing points"); //8
        logFlowPoint("Enter credit card information for purchasing points"); //9
        logFlowPoint("Verify that the 'Thank You' page appears for purchasing dlc content"); //10
        logFlowPoint("Purchase dlc with points through the 'Thank You' page and verify that the dlc content is downloadable"); //11

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        new MainMenu(driver).clickMaximizeButton();
        logPassFail(MacroPurchase.purchaseEntitlement(driver, baseGame), true);

        //3
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //4
        logPassFail(gameLibrary.isGameTileVisibleByOfferID(baseGameOfferID), true);

        //5
        GameSlideout baseGameSlideout = new GameTile(driver, baseGameOfferID).openGameSlideout();
        logPassFail(baseGameSlideout.verifyGameTiteByOfferID(baseGameOfferID), true);

        //6
        baseGameSlideout.clickExtraContentTab();
        SlideoutExtraContent extraContent = new SlideoutExtraContent(driver);
        logPassFail(extraContent.verifyBaseGameExtraContent(dlcOfferID), true);

        //7
        extraContent.clickExtraContentButton(dlcOfferID);
        PurchasePointsPackageDialog purchasePointsPackageDialog = new PurchasePointsPackageDialog(driver);
        purchasePointsPackageDialog.waitForPurchasePointsPackageDialogToLoad();
        logPassFail(Waits.pollingWait(() -> purchasePointsPackageDialog.verifyPurchasePointsPackageDialogReached()), true);

        //8
        // click complete order button for selected points by default
        purchasePointsPackageDialog.clickReviewOrderButton();
        PaymentInformationPage paymentInfoPage = new PaymentInformationPage(driver);
        paymentInfoPage.waitForPaymentInfoPageToLoad();
        logPassFail(Waits.pollingWait(() -> paymentInfoPage.verifyPaymentInformationReached()), true);

        //9
        logPassFail(MacroPurchase.completePurchaseWithPurchasingPoints(driver), true);

        //10
        ThankYouPage thankYouPage = new ThankYouPage(driver);
        logPassFail(Waits.pollingWait(() -> thankYouPage.verifyPurchasingPointsThankYouPageReached()), true);

        //11
        // purchase extra content using points purchased by
        thankYouPage.clickPayNowButton();
        Waits.pollingWait(() -> !thankYouPage.verifyPayNowButtonVisible());
        thankYouPage.clickCloseButton();
        logPassFail(Waits.pollingWait(() -> extraContent.verifyExtraContentIsDownloadable(dlcOfferID)), true);

        softAssertAll();
    }
}