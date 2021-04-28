package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.YouNeedOriginDialog;
import com.ea.originx.automation.lib.pageobjects.store.BrowseGamesPage;
import com.ea.originx.automation.lib.pageobjects.store.StoreHomeTile;
import com.ea.vx.originclient.utils.Waits;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to the store
 *
 * @author palui
 */
public final class MacroStore {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     */
    private MacroStore() {
    }

    /**
     * Checks if store tiles in browse games page have title, packart and price
     *
     * @param driver
     * @return List of entitlement which do not have title, parkart or/and price.
     */
    public static List<String> checkStoreTilesTitlePackartPriceVisible(WebDriver driver) {

        List<String> failedEntitlements = new ArrayList<>();
        String type;

        for (StoreHomeTile storeTile : new BrowseGamesPage(driver).getLoadedStoreGameTiles()) {

            final String productTitle = storeTile.getTileTitle();
            final String message = productTitle + " does not have ";
            if (!storeTile.verifyTileTitleVisible()) {
                type = "title";
                failedEntitlements.add(message + type);
                _log.debug(message + type);
            }
            if (!storeTile.verifyTileImageVisible()) {
                type = "packart";
                failedEntitlements.add(message + type);
                _log.debug(message + type);
            }
        }
        return failedEntitlements;
    }

    /**
     * Goes through download flow after entitling an offer.
     *
     * @param driver
     */
    public static void downloadEntitlement(WebDriver driver) {
        CheckoutConfirmation confirmation = new CheckoutConfirmation(driver);
        confirmation.clickDownload();
        DownloadDialog dlDialog = new DownloadDialog(driver);
        if (dlDialog.isDialogVisible()) {
            dlDialog.clickNext();
            DownloadEULASummaryDialog dlEULADialog = new DownloadEULASummaryDialog(driver);
            dlEULADialog.setLicenseAcceptCheckbox(true);
            dlEULADialog.clickNextButton();
        }
    }

    /**
     * Goes through the download flow of Origin Setup when attempting download from PDP page(does not actually start the download)
     *
     * @param driver
     */
    public static void downloadOriginSetup(WebDriver driver) throws Exception {
        //handle 'You Need Origin' dialog
        YouNeedOriginDialog youNeedOriginDialog = new YouNeedOriginDialog(driver);
        if (youNeedOriginDialog.waitIsVisible()) {
            youNeedOriginDialog.clickDownloadOriginButton();
            youNeedOriginDialog.clickCloseCircle();
        }
        Waits.sleep(2000); // waiting for file to be displayed in page footer
    }
}
