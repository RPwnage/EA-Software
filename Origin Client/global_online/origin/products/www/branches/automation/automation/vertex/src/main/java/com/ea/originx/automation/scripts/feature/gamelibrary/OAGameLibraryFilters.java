package com.ea.originx.automation.scripts.feature.gamelibrary;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.FilterMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * @author nvarthakavi
 */
public class OAGameLibraryFilters extends EAXVxTestTemplate {

    @TestRail(caseId = 3087352)
    @Test(groups = {"gamelibrary", "gamelibrary_smoke", "client_only", "allfeaturesmoke"})
    public void testGameLibraryFilters(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlementDipSmall = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementNameDipSmall = entitlementDipSmall.getName();
        final String entitlementOfferIdDipSmall = entitlementDipSmall.getOfferId();

        final EntitlementInfo entitlementNonDipSmall = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_SMALL);
        final String entitlementNameNonDipSmall = entitlementNonDipSmall.getName();
        final String entitlementOfferIdNonDipSmall = entitlementNonDipSmall.getOfferId();

        final EntitlementInfo entitlementDipLarge = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final String entitlementNameDipLarge = entitlementDipLarge.getName();
        final String entitlementOfferIdDipLarge = entitlementDipLarge.getOfferId();

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlementDipSmall, entitlementNonDipSmall, entitlementDipLarge);
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to '%s','%s' and '%s' ",
                username, entitlementNameDipSmall, entitlementNameNonDipSmall, entitlementNameDipLarge)); //1
        logFlowPoint(String.format("Navigate to the Game Library and verify games '%s','%s' and '%s' appears in the game library",
                entitlementNameDipSmall, entitlementNameNonDipSmall, entitlementNameDipLarge)); //2
        logFlowPoint(String.format("Download and Install the game '%s'", entitlementNameDipSmall)); //3
        logFlowPoint(String.format("Set the filter to Installed and Verify only '%s' game appears in the Game Library",
                entitlementNameDipSmall));//4
        logFlowPoint(String.format("Reset the filters and set filter by Name to '%s' and Verify only '%s' game appears in the Game Library",
                entitlementNameNonDipSmall, entitlementNameNonDipSmall));//5
        logFlowPoint(String.format("Reset the filters and add '%s' to Favourites and set filter to Favourites and Verify only '%s' game appears in the Game Library",
                entitlementNameDipLarge, entitlementNameDipLarge));//6
        logFlowPoint(String.format("Reset the filters and start downloading '%s' and set filter to Downloading and Verify only '%s' game appears in the Game Library",
                entitlementNameDipLarge, entitlementNameDipLarge));//7

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass(String.format("Verified login successful as user %s", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as user %s", username));
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        gameLibrary.waitForPage();
        GameTile gameTileDipSmall = new GameTile(driver, entitlementOfferIdDipSmall);
        GameTile gameTileNonDipSmall = new GameTile(driver, entitlementOfferIdNonDipSmall);
        GameTile gameTileDipLarge = new GameTile(driver, entitlementOfferIdDipLarge);
        boolean gameTilesVisible = gameTileDipSmall.isGameTileVisible() && gameTileNonDipSmall.isGameTileVisible() && gameTileDipLarge.isGameTileVisible();
        if (gameTilesVisible) {
            logPass(String.format("Verified successful navigation to Game Library with games '%s','%s' and '%s' in it",
                    entitlementNameDipSmall, entitlementNameNonDipSmall, entitlementNameDipLarge));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or cannot locate games '%s','%s' and '%s'",
                    entitlementNameDipSmall, entitlementNameNonDipSmall, entitlementNameDipLarge));
        }

        //3
        boolean entitlementDownloaded = MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferIdDipSmall);
        if (entitlementDownloaded) {
            logPass(String.format("Verified game '%s' is the successfully downloaded", entitlementNameDipSmall));
        } else {
            logFailExit(String.format("Failed: game '%s' is not successfully downloaded", entitlementNameDipSmall));
        }

        //4
        gameLibrary.openFilterMenu();
        FilterMenu filterMenu = new FilterMenu(driver);
        filterMenu.setInstalled();
        boolean gameTileDipSmallVisible = gameLibrary.verifyGameLibraryHasExpectedGames(entitlementOfferIdDipSmall);
        if (gameTileDipSmallVisible) {
            logPass(String.format("Verified Game Library with only game '%s' in it when filter is set to Installed",
                    entitlementNameDipSmall));
        } else {
            logFailExit(String.format("Failed: Game Library does not have only the game '%s' when filter is set to Installed",
                    entitlementNameDipSmall));
        }

        //5
        filterMenu.clearAllFilters();
        filterMenu.enterFilterText(entitlementNameNonDipSmall);
        boolean gameTileNonDipSmallVisible = gameLibrary.verifyGameLibraryHasExpectedGames(entitlementOfferIdNonDipSmall);
        if (gameTileNonDipSmallVisible) {
            logPass(String.format("Verified Game Library with only game '%s' in it when Search filter is set to '%s'",
                    entitlementNameNonDipSmall, entitlementNameNonDipSmall));
        } else {
            logFailExit(String.format("Failed: Game Library does not have only the game '%s' when Search filter is set to '%s'",
                    entitlementNameNonDipSmall, entitlementNameNonDipSmall));
        }

        //6
        filterMenu.clearAllFilters();
        gameLibrary.clearAllFavoritesIfExists();
        gameTileDipLarge.addToFavorites();
        filterMenu.setFavorites();
        boolean gameTileDipLargeVisible = gameLibrary.verifyGameLibraryHasExpectedGames(entitlementOfferIdDipLarge);
        if (gameTileDipLargeVisible) {
            logPass(String.format("Verified Game Library with only game '%s' in it when filter is set to Favourites",
                    entitlementNameDipLarge));
        } else {
            logFailExit(String.format("Failed: Game Library does not have only the game '%s' in it when filter is set to Favourites",
                    entitlementNameDipLarge));
        }
        gameLibrary.clearAllFavoritesIfExists();

        //7
        filterMenu.clearAllFilters();
        boolean entitlementDownloading = MacroGameLibrary.startDownloadingEntitlement(driver, entitlementOfferIdDipLarge);
        filterMenu.setDownloading();
        boolean gameTileDipLargeDownloadingVisible = gameLibrary.verifyGameLibraryHasExpectedGames(entitlementOfferIdDipLarge) && entitlementDownloading;
        if (gameTileDipLargeDownloadingVisible) {
            logPass(String.format("Verified Game Library with only game '%s' in it when filter is set to Downloading",
                    entitlementNameDipLarge));
        } else {
            logFailExit(String.format("Failed: Game Library does not have only the game '%s' in it when filter is set to Downloading",
                    entitlementNameDipLarge));
        }

        softAssertAll();

    }
}
