package com.ea.originx.automation.scripts.feature.checkout;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.SlideoutExtraContent;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests if expansion for base game can be purchasable and downloadable by
 * purchasing base game using new account
 *
 * @author mdobre
 */
public class OAPurchaseEntitlementExpansion extends EAXVxTestTemplate {

    @TestRail(caseId = 3068469)
    @Test(groups = {"checkout_smoke", "checkout", "int_only", "allfeaturesmoke"})
    public void testPurchaseEntitlement(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        Battlefield4 bf4_standard_entitlement = new Battlefield4();
        final String bf4_standard_entitlement_name = bf4_standard_entitlement.getName();
        final String bf4_standard_entitlement_offerId = bf4_standard_entitlement.getOfferId();
        final String expansionOfferId = Battlefield4.BF4_FINAL_STAND_OFFER_ID;
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
                
        logFlowPoint("Launch Origin and register as a new user");  //1
        logFlowPoint("Navigate to GDP Page"); //2
        logFlowPoint("Purchase BF4 Premium entitlement"); //3
        logFlowPoint("Navigate to game library"); //4
        logFlowPoint("Verify Game appears in the game library after purchase"); //5
        logFlowPoint("Slideout for Battlefield 4 is successfully opened"); //6
        logFlowPoint("Verify Extra Content tab exists for BF4 standard entitlement"); //7
        logFlowPoint("Verify BF4 expansion 'Final Stand' is found by offerID under Extra Content tab"); //8
        logFlowPoint("Purchase BF4 expansion 'Final Stand'"); //9
        logFlowPoint("Verify BF4 expansion 'Final Stand' entitlement is downloadable");  //10

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        if(isClient) {
            new MainMenu(driver).clickMaximizeButton();
        }
        logPassFail(MacroGDP.loadGdpPage(driver, bf4_standard_entitlement), false);

        //3
        logPassFail(MacroPurchase.purchaseEntitlement(driver, bf4_standard_entitlement), true);

        //4
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //5
        logPassFail(gameLibrary.isGameTileVisibleByName(bf4_standard_entitlement_name), true);

        //6
        GameTile bf4StandardTile = new GameTile(driver, bf4_standard_entitlement_offerId);
        GameSlideout bf4StandardSlideout = bf4StandardTile.openGameSlideout();
        logPassFail(bf4StandardSlideout.verifyGameTitle(bf4_standard_entitlement_name), true);

        //7
        logPassFail(bf4StandardSlideout.verifyExtraContentNavLinkVisible(), true);

        //8
        bf4StandardSlideout.clickExtraContentTab();
        SlideoutExtraContent extraContent = new SlideoutExtraContent(driver);
        logPassFail(extraContent.verifyBaseGameExtraContent(expansionOfferId), true);

        //9
        extraContent.clickExtraContentButton(expansionOfferId);
        logPassFail(MacroPurchase.completePurchaseAndCloseThankYouPage(driver), true);

        //10
        if (isClient) {
            logPassFail(Waits.pollingWait(() -> extraContent.verifyExtraContentIsDownloadable(expansionOfferId)), true);
        } else {
            logPassFail(Waits.pollingWait(() -> extraContent.verifyExtraContentCanBePlayOnOrigin(expansionOfferId)), true);
        }

        softAssertAll();
    }
}