package com.ea.originx.automation.scripts.checkout;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.BaseGameMessageDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.SlideoutExtraContent;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test purchasing BioWare points and verifying the purchase was successful and that
 * the points are showing up on the account.
 *
 * NEEDS UPDATE TO GDP
 *
 * @author caleung
 */
public class OAPurchaseBioWarePoints extends EAXVxTestTemplate {

    @TestRail(caseId = 3068172)
    @Test(groups = {"cia", "checkout", "os_smoke"})
    public void testPurchaseBioWarePoints(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ME3);

        int expectedBioWarePointsBalance = 1600;

        logFlowPoint("Log into Origin as a newly registered user."); // 1
        logFlowPoint("Purchase an entitlement that has extra content that requires 'BioWare Points' to purchase."); // 2
        logFlowPoint("Navigate to the 'BioWare Points' page, purchase 'BioWare Points' and verify checkout is successful."); // 3
        logFlowPoint("Navigate to the 'Game Library', click on the entitlement to open the slideout to go to the 'Extra Content' tab."); // 4
        logFlowPoint("Choose an extra content that requires 'BioWare Points' to purchase, start the purchase flow, and verify the points purchased are showing."); // 5

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully created user and logged in as: " + userAccount.getUsername());
        } else {
            logFailExit("Failed to create user.");
        }

        // 2
        if (MacroPurchase.purchaseEntitlement(driver, entitlement)) {
            logPass("Successfully purchased an entitlement that has extra content that requires 'BioWare Points' to purchase.");
        } else {
            logFailExit("Failed to purchase an entitlement that has extra content that requires 'BioWare Points' to purchase.");
        }

        // 3
      //  MacroPDP.loadPdpPageBySearch(driver, "1600 BioWare Points");
        //  PDPHeroActionCTA pdpHeroCTA = new PDPHeroActionCTA(driver);
        //   pdpHeroCTA.verifyBuyButtonVisible();
        //   pdpHeroCTA.clickBuyButton();
        new BaseGameMessageDialog(driver, null, null).clickContinuePurchaseCTA();
        MacroPurchase.completePurchase(driver);
        if (MacroPurchase.handleThankYouPage(driver)) {
            logPass("Successfully purchased 1600 'BioWare Points'.");
        } else {
            logFailExit("Failed to purchase 1600 'BioWare Points'.");
        }

        // 4
        new NavigationSidebar(driver).gotoGameLibrary();
        new GameLibrary(driver).waitForPage();
        GameSlideout gameSlideout = new GameTile(driver, entitlement.getOfferId()).openGameSlideout();
        gameSlideout.waitForSlideout();
        Waits.pollingWait(() -> gameSlideout.verifyExtraContentNavLinkVisible());
        gameSlideout.clickExtraContentTab();
        SlideoutExtraContent slideoutExtraContent = new SlideoutExtraContent(driver);
        if (slideoutExtraContent.isExtraContentOpen()) {
            logPass("Verified 'Extra Content' page has been reached.");
        } else {
            logFailExit("Failed to verify 'Extra Content' page has been reached.");
        }

        // 5
        slideoutExtraContent.clickExtraContentButton("OFB-MASS:57550");
        int bioWarePointsBalance = Integer.parseInt(new ReviewOrderPage(driver).getBioWarePointsBalance());
        if (bioWarePointsBalance == expectedBioWarePointsBalance) {
            logPass("Verified points purchased show up.");
        } else {
            logFailExit("Failed to verify points show up.");
        }

        softAssertAll();
    }
}