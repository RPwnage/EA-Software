package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.FilterMenu;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.OADipSmallGame;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the game library filter menu
 *
 * @author nvarthakavi
 */
public class OAFilterTextClear extends EAXVxTestTemplate {

    @TestRail(caseId = 9967)
    @Test(groups = {"gamelibrary", "full_regression", "int_only"})
    public void testFilterTextClear(ITestContext context) throws Exception {
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final EntitlementInfo nonLargeDip = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_LARGE);
        final EntitlementInfo nonSmallDip = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.NON_DIP_SMALL);

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo largeDip = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_LARGE);
        final EntitlementInfo smallDip = new OADipSmallGame();
        String smallDipName = smallDip.getName();
        String largeDipName = largeDip.getName();
        String nonSmallDipName = nonSmallDip.getName();
        String nonLargeDipName = nonLargeDip.getName();
        UserAccount user = AccountManager.getEntitledUserAccount(largeDip, smallDip, nonLargeDip, nonSmallDip);

        logFlowPoint("Log into Origin with " + user.getUsername()); //1
        logFlowPoint("Navigate to Game Library" + (isClient ? "and install " + smallDipName : "")); //2
        logFlowPoint("Open the filter menu"); //3
        logFlowPoint("Enter filter text: " + smallDipName + " and verify a new bubble appears at the top of the page: 'Filter By: " + smallDipName + "'"); //4
        logFlowPoint("Select a pre-made filters: 'Purchased Games', 'Platform-PC', " + (isClient ? "'Installed'" : "") + " and verify that a new bubbles appear at the top of the page for the selected pre-made filter"); //5
        logFlowPoint("Verify that only " + smallDipName + " is visible in the gamelibrary"); //6
        logFlowPoint("Clear the text field by clicking the 'X' button and verify the text field is cleared and filter is appeared"); //7
        logFlowPoint("Clear" + (isClient ? "'Installed'" : "'Platform-PC'") + " filter by clicking the 'X' button on the bubble and verify the filter is removed"); //8
        logFlowPoint("Click 'Clear All' and verify all applied filter bubbles are cleared and verify all the games are visible"); //9
        logFlowPoint("Enter filter text:'No results' and verify 'No results' message appears"); //10

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        //Client window needs to be maximized for filter bubbles to appear in flow points
        if (isClient) {
            new MainMenu(driver).clickMaximizeButton();
        }
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        if (isClient) {
            MacroGameLibrary.downloadFullEntitlement(driver, smallDip.getOfferId());
        }
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);

        //3
        FilterMenu filterMenu = gameLibrary.openFilterMenu();
        logPassFail(filterMenu.isFilterMenuOpen(), true);

        //4
        filterMenu.enterFilterText(smallDipName);
        logPassFail(gameLibrary.isGameTileVisibleByName(smallDipName) && filterMenu.isFilterPillFilterByTextVisible(smallDipName), false);

        //5
        boolean isFilterTextPcPurchasedBubblesVisible;
        //Purchased and PC filters are applied both in case of Client and Browser. Installed Filter is only applied on Client
        int numberOfActiveFilters = 2 ;
        filterMenu.setPurchasedGames();
        filterMenu.setPCGame();
        if (isClient) {
            filterMenu.setInstalled();
            numberOfActiveFilters++;
            isFilterTextPcPurchasedBubblesVisible = filterMenu.isFilterPillFilterByTextVisible(smallDipName) &&
                    filterMenu.verifyFilterPillMultipleVisible("+ " + numberOfActiveFilters + " active filters");
        } else {
            isFilterTextPcPurchasedBubblesVisible = filterMenu.isFilterPillFilterByTextVisible(smallDipName) &&
                    filterMenu.verifyFilterPillMultipleVisible("+ " + numberOfActiveFilters + " active filters");
        }

        //When more than two filters are selected the first selected filter bubble is visible with a multiple active filter bubbles instead of individual filter bubbles
        logPassFail(isFilterTextPcPurchasedBubblesVisible, false);

        //6
        boolean isFilterTextMacPurchasedGamesInstallVisible = gameLibrary.isGameTileVisibleByName(smallDipName) && !gameLibrary.isGameTileVisibleByName(largeDipName)
                && !gameLibrary.isGameTileVisibleByName(nonLargeDipName) && !gameLibrary.isGameTileVisibleByName(nonSmallDipName);
        logPassFail(isFilterTextMacPurchasedGamesInstallVisible, false);

        //7
        filterMenu.clearTextField();
        boolean isTextFieldCleared = filterMenu.verifyFilterTextFieldIsCleared();
        boolean isRemoveFilterTextGamesVisible;
        if (isClient) {
            isRemoveFilterTextGamesVisible = gameLibrary.isGameTileVisibleByName(smallDipName) && !gameLibrary.isGameTileVisibleByName(largeDipName)
                    && !gameLibrary.isGameTileVisibleByName(nonLargeDipName) && !gameLibrary.isGameTileVisibleByName(nonSmallDipName);
        } else {
            isRemoveFilterTextGamesVisible = gameLibrary.isGameTileVisibleByName(smallDipName) && gameLibrary.isGameTileVisibleByName(largeDipName)
                    && gameLibrary.isGameTileVisibleByName(nonLargeDipName) && gameLibrary.isGameTileVisibleByName(nonSmallDipName);
        }
        logPassFail(isTextFieldCleared && isRemoveFilterTextGamesVisible, false);

        //8
        boolean isRemoveFilterBubbleVisible;
        if (isClient) {
            filterMenu.clearFilterPillBubbleInstalled();
        } else {
            filterMenu.clearFilterPillBubblePC();
        }
        isRemoveFilterBubbleVisible = gameLibrary.isGameTileVisibleByName(smallDipName) && gameLibrary.isGameTileVisibleByName(largeDipName)
                && gameLibrary.isGameTileVisibleByName(nonLargeDipName) && gameLibrary.isGameTileVisibleByName(nonSmallDipName);
        logPassFail(isRemoveFilterBubbleVisible, false);

        //9
        filterMenu.clearAllFilters();
        logPassFail(filterMenu.verifyNoActiveFilters(), false);

        //10
        filterMenu.enterFilterText("NoResults");
        logPassFail(gameLibrary.verifyNoResultsFoundTextVisible(), false);

        softAssertAll();
    }
}
