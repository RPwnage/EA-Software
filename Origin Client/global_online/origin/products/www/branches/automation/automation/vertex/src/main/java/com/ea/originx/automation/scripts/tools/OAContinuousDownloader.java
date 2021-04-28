package com.ea.originx.automation.scripts.tools;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.originx.automation.lib.macroaction.DownloadOptions;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.UnexpectedErrorDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;

import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * The test script to download all the entitlements in the Origin catalogue.
 *
 * @author glivingstone
 */
public class OAContinuousDownloader extends EAXVxTestTemplate {

    @Test(groups = {"continuousdownloader", "client_only"})
    public void runContinuousDownloader(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.CONTINUOUS_DOWNLOADER_ALL);

        final WebDriver driver = startClientObject(context, client);
        MacroLogin.startLogin(driver, user);

        NavigationSidebar navBar = new NavigationSidebar(driver);
        navBar.gotoGameLibrary();
        sleep(90000); // Long sleep to load over 200 entitlements

        List<String> allEntOfferIDs = getEntitlementSlice(driver);

        for (String offerID : allEntOfferIDs) {
            logFlowPoint("Downloading: " + new GameTile(driver, offerID).getTitle());
        }

        List<String> allProcesses = ProcessUtil.getRunningProcessIDs(client);

        UnexpectedErrorDialog unexpectedErrorDialog = new UnexpectedErrorDialog(driver);
        DownloadOptions options = new DownloadOptions().setDesktopShortcut(false).setStartMenuShortcut(false).setUncheckExtraContent(false);
        for (String offerID : allEntOfferIDs) {
            String gameName = new GameTile(driver, offerID).getTitle();
            try {
                if (MacroGameLibrary.downloadFullEntitlement(driver, offerID, options)) {
                    logPass("Successfully downloaded " + gameName);
                } else {
                    logFail("Could not download " + gameName + ", please download manually.");
                }
            } catch (Exception e) {
                logFail("Error trying to download " + gameName + ", please download manually.");
            }
            ProcessUtil.killProcessesNotInList(client, allProcesses);
            // If we kill processes that are installing an error dialog appears
            if (unexpectedErrorDialog.isOpen()) {
                unexpectedErrorDialog.close();
                sleep(1000);
            }
        }
        softAssertAll();
    }

    /**
     * Get a slice of the entitlements from the account. This will grab every
     * nth entitlement offerID, where n is NUM_SECTIONS. We only grab a slice
     * every time to reduce the total number of tiles to test.
     *
     * @param driver The driver to use for this test
     * @return A list of offerIDs to download
     */
    private List<String> getEntitlementSlice(WebDriver driver) {
        final String[] WORD_BLACKLIST = {"Beta", "Rift", "Defiance", "The Game of Life"};
        final int NUM_SECTIONS = 9;

        GameLibrary gameLibrary = new GameLibrary(driver);
        List<WebElement> allTiles = gameLibrary.getGameTileElements();

        List<String> tileSlice = new ArrayList<>();

        int dayOfYear = Calendar.getInstance().get(Calendar.DAY_OF_YEAR);
        // Choose which of the 9 sections to use
        int daySplice = dayOfYear % NUM_SECTIONS;

        String offerID, gameTitle;
        GameTile gameTile;

        while (daySplice < allTiles.size()) {
            offerID = allTiles.get(daySplice).getAttribute("offerid");
            gameTile = new GameTile(driver, offerID);

            gameTitle = gameTile.getTitle();
            if (!StringHelper.containsAnyIgnoreCase(gameTitle, WORD_BLACKLIST)) {
                tileSlice.add(offerID);
            }
            daySplice += NUM_SECTIONS;
        }
        return tileSlice;
    }
}
