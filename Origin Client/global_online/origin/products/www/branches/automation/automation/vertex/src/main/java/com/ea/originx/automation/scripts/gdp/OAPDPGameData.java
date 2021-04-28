package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroStore;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.store.StoreGameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests if game data such as title, packart and price visible on Browse Games
 * page. Also check if price on Browse Games page match with one on the PDP page
 * and make sure if right title, game rating image, packart and description are
 * visible in the PDP page.
 *
 * NEEDS UPDATE TO GDP
 *
 * @author rchoi
 */
public class OAPDPGameData extends EAXVxTestTemplate {

    @TestRail(caseId = 1016728)
    @Test(groups = {"pdp", "services_smoke", "allfeaturesmoke"})
    public void OAPDPGameData(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String userName = userAccount.getUsername();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.SIMS4_STANDARD);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferId = entitlement.getOfferId();

        logFlowPoint("Login to Origin with newly registered user"); //1
        logFlowPoint("Navigate to Browse Games page and verify all store files have title, packart and price"); //2
        logFlowPoint("Get price for one entitlement on browse Games page and navigate to PDP page for the entitlement"); //3
        logFlowPoint("Verify the price in Browse Games page matches with the price in the PDP page"); //4
        logFlowPoint("Verify the packart of entitlement existS in the PDP page"); //5
        logFlowPoint("Verify the name of entitlement existS in the PDP page"); //6
        logFlowPoint("Verify the header and truncated body description exist in the PDP page"); //7
        logFlowPoint("Verify the game rating image exists in the PDP page"); //8

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user." + userName);
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        new NavigationSidebar(driver).gotoBrowseGamesPage();
        if (MacroStore.checkStoreTilesTitlePackartPriceVisible(driver).isEmpty()) {
            logPass("Verified all store files have title, packart and price exist");
        } else {
            logFailExit("Not all store files have title, packart and price ");
        }

        // 3
        float sellingPrice = new StoreGameTile(driver, entitlementOfferId).getPrice();
//        if (MacroPDP.loadPdpPage(driver, entitlement)) {
//            logPass("Navigated to " + entitlementName + "'s PDP page");
//        } else {
//            logFailExit("Failed to navigate to " + entitlementName + "'s PDP page");
//        }

//        // 4
//        if (new PDPHeroActionCTA(driver).isPrice(sellingPrice)) {
//            logPass("Verified the price in Browse Games page matches with price exist in the PDP page");
//        } else {
//            logFail("Failed: the price in Browse Games page and in PDP page are different");
//        }
//
//        // 5
//        PDPHeroPackartRating pdpHeroPackartRating = new PDPHeroPackartRating(driver);
//        if (pdpHeroPackartRating.verifyPackArtVisible()) {
//            logPass("Verified the packart of entitlement exist in the PDP page");
//        } else {
//            logFail("Failed: The packart is not visible");
//        }
//
//        // 6
//        PDPHeroDescription pdpHeroDescription = new PDPHeroDescription(driver);
//        if (pdpHeroDescription.verifyGameTitle(entitlementName)) {
//            logPass("Verified the name of entitlement exist in the PDP page");
//        } else {
//            logFail("Failed: the entitlement name does not match");
//        }
//
//        // 7
//        if (pdpHeroDescription.verifyDescriptions()) {
//            logPass("Verified text for the header and truncated Body description exist in the PDP page");
//        } else {
//            logFail("Failed to verify text for the header and truncated body description exist in the PDP page");
//        }
//
//        // 8
//        if (pdpHeroPackartRating.verifyPDPHeroGameRatingVisible()) {
//            logPass("Verified the game rating image exists");
//        } else {
//            logFail("Failed to check the game rating image");
//        }
        softAssertAll();
    }
}
