package com.ea.originx.automation.scripts.tools;

import com.ea.originx.automation.lib.helpers.csv.OriginEntitlementHelper;
import com.ea.originx.automation.lib.helpers.csv.OriginEntitlementReader;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.ExtraContentGameCard;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.net.helpers.CrsHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * To purchase all games through their GDP page
 *
 * @author svaghayenegar
 */
public class OAGDPPurchaseAllGames extends EAXVxTestTemplate {

    public void testGDPPurchaseAllGames(ITestContext context, boolean isSubscriber) throws Exception {

        // Get the report id and the current test-result-id to get the index number to load the corresponding GDP
        String reportID = CrsHelper.getReportId();
        String testCaseName;

        if (isSubscriber) {
            testCaseName = "com.ea.originx.automation.scripts.tools.parameterized.OAGDPPurchaseAllGamesSubscriber";
        } else {
            testCaseName = "com.ea.originx.automation.scripts.tools.parameterized.OAGDPPurchaseAllGamesNonSubscriber";
        }

        // Get the TestCase Id from the testcase Name
        int testCaseID = CrsHelper.getTestCaseId(testCaseName);
        int index = Integer.parseInt(resultLogger.getCurrentTestResultId()) - CrsHelper.getTestResultId(reportID, testCaseID, true);
        OriginEntitlementHelper originEntitlementHelper = OriginEntitlementReader.getEntitlementInfo(index);

        // Entitlement details
        String entitlementName = originEntitlementHelper.getEntitlementName();
        String partialPDPUrl = originEntitlementHelper.getEntitlementPartialPDPUrl();
        boolean isBaseGameNeeded = originEntitlementHelper.isBaseGameNeeded();
        boolean isBundle = originEntitlementHelper.isBundle();
        String gameLibraryImageName = originEntitlementHelper.getGameLibraryImageName();

        EntitlementInfo entitlement = originEntitlementHelper.getAsEntitlementInfo();


        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final boolean isExpansionOrAddon = partialPDPUrl.contains("/expansion/") || partialPDPUrl.contains("/addon/");
        final boolean canBePuchasedMultipleTimes = entitlementName.contains("Battlepack") ||
                partialPDPUrl.contains("/currency") ||
                partialPDPUrl.contains("/game-time/") ||
                (!isBaseGameNeeded && isExpansionOrAddon);


        logFlowPoint("Log into Origin as newly registered account."); // 1

        if (isSubscriber) {
            logFlowPoint("Purchase Origin Access."); // 2
        }

        if (isBaseGameNeeded) {
            for (EntitlementInfo baseGame : originEntitlementHelper.getBaseGamesEntitlementInfo()) {
                logFlowPoint(String.format("Buy base game [%s], complete the purchase flow and close the thank you modal.", baseGame.getName())); // 3
            }
        }

        logFlowPoint(String.format("Navigate to GDP page of [%s], verify it load, complete the purchase flow and close the thank you modal.", entitlementName)); // 4

        if (!canBePuchasedMultipleTimes) {
            logFlowPoint("Navigate to 'Game Library' and verify the page is navigated to 'Game Library'."); // 5

            if (isExpansionOrAddon) {
                logFlowPoint("Click on the base game bought, verify its slideout opens, should be only 1 game in library"); // 6
                logFlowPoint("Click on extra content, and verify game has extra content game cards"); // 7
                if (isBundle) {
                    for (OriginEntitlementHelper bundleEntitlementHelper : originEntitlementHelper.getBundlesEntitlementInfo()) {
                        logFlowPoint(String.format("Verify extra content with offer-id[%s] for base game exist in extra content", bundleEntitlementHelper.getEntitlementOfferId())); // 8
                    }
                } else {
                    logFlowPoint(String.format("Verify extra content with offer-id[%s] for base game exist in extra content", entitlement.getOfferId())); // 9
                }
            } else {
                if (isBundle) {
                    logFlowPoint("Verify all the games in the bundle are now in 'Game Library'."); // 10
                } else {
                    logFlowPoint("Verify the purchased game is now in 'Game Library'."); // 11
                }

            }
        }

        WebDriver driver = startClientObject(context, client);

        // 1
        if (isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        // 2
        if (isSubscriber) {
            logPassFail(MacroOriginAccess.purchaseOriginAccess(driver), true);
        }

        // 3
        if (isBaseGameNeeded) {
            for (EntitlementInfo baseGame : originEntitlementHelper.getBaseGamesEntitlementInfo()) {
                logPassFail(MacroPurchase.purchaseEntitlement(driver, baseGame), true);
            }
        }

        // 4
        logPassFail(MacroPurchase.purchaseEntitlement(driver, entitlement), true);


        if (!canBePuchasedMultipleTimes) {
            new NavigationSidebar(driver).gotoGameLibrary();
            GameLibrary gameLibrary = new GameLibrary(driver);
            logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);


            if (isExpansionOrAddon) {
                // 7
                gameLibrary.getGameTileElements().get(0).click();
                GameSlideout gameSlideout = new GameSlideout(driver);
                logPassFail(gameSlideout.waitForSlideout(), true);

                if (!isBaseGameSims(originEntitlementHelper)) {
                    // 8
                    gameSlideout.clickExtraContentTab();
                    logPassFail(gameSlideout.verifyExtraContentInExtraContentTab(), true);

                    // 9
                    if (isBundle) {
                        for (OriginEntitlementHelper bundleEntitlementHelper : originEntitlementHelper.getBundlesEntitlementInfo()) {
                            ExtraContentGameCard extraContentGameCard = new ExtraContentGameCard(driver, bundleEntitlementHelper.getEntitlementOfferId());
                            extraContentGameCard.scrollToGameCard();
                            boolean verifyEntitlement = extraContentGameCard.verifyDownloadButtonVisible() ||
                                    extraContentGameCard.verifyDetailsButtonVisible();
                            logPassFail(verifyEntitlement, false);
                        }
                    } else { // 10
                        ExtraContentGameCard extraContentGameCard = new ExtraContentGameCard(driver, entitlement.getOfferId());
                        extraContentGameCard.scrollToGameCard();
                        boolean verifyEntitlement = extraContentGameCard.verifyDownloadButtonVisible() ||
                                extraContentGameCard.verifyDetailsButtonVisible();
                        logPassFail(verifyEntitlement, false);
                    }
                }
            } else {
                // 11
                if (isBundle) {
                    for (OriginEntitlementHelper bundleEntitlementHelper : originEntitlementHelper.getBundlesEntitlementInfo()) {
                        logPassFail(gameLibrary.isGameTileVisibleByName(bundleEntitlementHelper.getGameLibraryImageName()), false);
                    }
                } else { // 12
                    logPassFail(gameLibrary.isGameTileVisibleByName(gameLibraryImageName), true);
                }
            }
        }

        softAssertAll();
    }

    private boolean isBaseGameSims(OriginEntitlementHelper originEntitlementHelper) {
        for (EntitlementInfo entitlement : originEntitlementHelper.getBaseGamesEntitlementInfo()) {
            if (entitlement.getName().toLowerCase().contains("sims"))
                return true;
        }
        return false;
    }
}
