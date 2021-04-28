package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.*;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.DownloadOptions.DownloadTrigger;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.*;
import com.ea.originx.automation.lib.pageobjects.store.DemoTile;
import com.ea.originx.automation.lib.pageobjects.store.DemosAndBetasPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoConstants;
import java.awt.AWTException;
import java.io.IOException;
import java.lang.invoke.MethodHandles;
import java.text.ParseException;
import java.util.List;
import java.util.ArrayList;
import java.util.Set;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Macro class containing static methods for multi-step actions related to the 'Game Library' page.
 *
 * @author glivingstone
 */
public final class MacroGameLibrary {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    private MacroGameLibrary() {
    }

    /**
     * Downloads the full entitlement, assuming the 'Game Library' page has been opened.
     *
     * @param driver     Selenium WebDriver
     * @param entOfferID The offerID of the entitlement to download
     * @param options    {@link DownloadOptions}<br>
     *                   UI options:<br>
     *                   - desktopShortcut if true, check the 'Desktop Shortcut' check box. If
     *                   false, un-check<br>
     *                   - startMenuShortcut if true, check the 'Start Menu Shortcut' check box.
     *                   If false, un-check<br>
     *                   - uncheckExtraContent if true, un-check the 'Extra Content' check boxes.
     *                   If false, do not un-check<br>
     *                   Flow options:<br>
     *                   - installFromDisc if true, we are installing from (ITO) disc and a
     *                   download preparation dialog should have already been opened. If false,
     *                   initiate download from the game tile's context menu <br>
     *                   - stopAtEULA if true, stop download when the EULA dialog appears. If
     *                   false, continue to end of install<br>
     * @return true if the entitlement successfully completes the download and installation, false otherwise
     * @throws AWTException
     */
    public static boolean downloadFullEntitlement(WebDriver driver, String entOfferID, DownloadOptions options) throws AWTException {
        if (!startDownloadingEntitlement(driver, entOfferID, options)) {
            return false;
        } else if (options.stopAtEULA) { // No need to continue if we have stopped at a EULA dialog
            return true;
        }
        return finishDownloadingEntitlement(driver, entOfferID);

    }
    
    /**
     * Downloads the full entitlement with selectedDLCs, assuming the 'Game Library' page has been opened.
     *
     * @param driver     Selenium WebDriver
     * @param entOfferID The offerID of the entitlement to download
     * @param options    {@link DownloadOptions}<br>
     *                   UI options:<br>
     *                   - desktopShortcut if true, check the 'Desktop Shortcut' check box. If
     *                   false, un-check<br>
     *                   - startMenuShortcut if true, check the 'Start Menu Shortcut' check box.
     *                   If false, un-check<br>
     *                   - uncheckExtraContent if true, un-check the 'Extra Content' check boxes.
     *                   If false, do not un-check<br>
     *                   Flow options:<br>
     *                   - installFromDisc if true, we are installing from (ITO) disc and a
     *                   download preparation dialog should have already been opened. If false,
     *                   initiate download from the game tile's context menu <br>
     *                   - stopAtEULA if true, stop download when the EULA dialog appears. If
     *                   false, continue to end of install<br>
     * @pram dlcList     The list of offer ID of DLCs to download.
     * @return true if the entitlement and selected DLCs successfully completes the download and installation, false otherwise
     * @throws AWTException
     */
    public static boolean downloadFullEntitlementWithSelectedDLC(WebDriver driver, String entOfferID, DownloadOptions options, List<String> dlcList) throws AWTException {
        if(!MacroGameLibrary.startDownloadingEntitlement(driver, entOfferID, new DownloadOptions(), dlcList)) {
            _log.debug("Fail to start downloading entitlement");
            return false;
        }
        if(!MacroGameLibrary.finishDownloadingEntitlement(driver, entOfferID)) {
            _log.debug("Fail to complete downloading entitlement");
            return false;
        }
        if(!MacroGameLibrary.finishDownloadingSelectedDLC(driver, dlcList)) {
            _log.debug("Fail to complete downloading dlc(s)");
            return false;
        }
        return true;
    }

    /**
     * Completely download the given entitlement with the start menu and desktop
     * shortcuts enabled, and disable all extra content. Assumes the 'Game
     * Library' page has been opened.
     *
     * @param driver  Selenium WebDriver
     * @param offerID The offerID of the entitlement to download
     * @return true if the entitlement is completely downloaded and installed,
     * false otherwise
     * @throws AWTException
     */
    public static boolean downloadFullEntitlement(WebDriver driver, String offerID) throws AWTException {
        return downloadFullEntitlement(driver, offerID, new DownloadOptions());
    }

    /**
     * Start downloading the entitlement, assuming the 'Game Library' page has been opened.
     *
     * @param driver                        Selenium WebDriver for the client
     * @param entOfferID                    The offerID of the entitlement to download
     * @param options                       {@link DownloadOptions}<br>
     *                                      UI options:<br>
     *                                      - desktopShortcut if true, check the 'Desktop Shortcut' check box. If
     *                                      false, un-check<br>
     *                                      - startMenuShortcut if true, check the 'Start Menu Shortcut' check box.
     *                                      If false, un-check<br>
     *                                      - uncheckExtraContent if true, un-check the 'Extra Content' check boxes
     *                                      (can use UncheckExtraContentExceptions parameter to check some back). If
     *                                      false, do not un-check (and the UncheckExtraContentExceptions parameter
     *                                      is ignored)<br>
     *                                      Flow options:<br>
     *                                      - installFromDisc if true, we are installing from (ITO) disc and a
     *                                      download preparation dialog should have already been opened. If false,
     *                                      initiate download from the game tile's context menu <br>
     *                                      - stopAtEULA if true, stop downlaod when the EULA dialog appears. If
     *                                      false, continue to end of install<br>
     * @param UncheckExtraContentExceptions If options specify all extra
     *                                      contents to be unchecked, this list of offer-ids will then be checked
     *                                      back. Useful when only a subset (often one) of the extra contents should
     *                                      be checked
     * @return true if the entitlement is downloading or the entitlement is not
     * preparing or downloading, false otherwise
     * @throws AWTException
     */
    public static boolean startDownloadingEntitlement(WebDriver driver, String entOfferID,
                                                      DownloadOptions options, List<String> UncheckExtraContentExceptions)
            throws AWTException {
        GameTile gameTile = new GameTile(driver, entOfferID);
        switch (options.downloadTrigger) {
            case INSTALL_FROM_DISC:
                // A download preparation dialog has been opened. Switch back to main page to allow access to gameTile
                Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
                break;
            case DOWNLOAD_FROM_GAME_TILE:
                gameTile.startDownload();
                break;
            case DOWNLOAD_FROM_GAME_SLIDEOUT_COG_WHEEL:
                new GameSlideout(driver).getCogwheel().download();
                break;
            default:
                throw new RuntimeException("Error creating Download Options object for Download Trigger enum:" + options.downloadTrigger);
        }

        // Handle download preparation dialogs
        if (!handleDialogs(driver, entOfferID, options, UncheckExtraContentExceptions)) {
            return false;
        } else if (options.stopAtEULA) { // No need to continue if we have stopped at a EULA dialog
            return true;
        }

        // The download may complete very quickly. If it's downloading, we are
        // good. If it's not downloading, we need to check if it has started. If
        // it's downloadable or preparing, we have failed to commence the
        // download.
        if (gameTile.isInstalling()) {
            _log.debug("entitlement is installing");
            return true;
        } else if (gameTile.waitForDownloading(5)) {
            _log.debug("entitlement is downloading");
            return true;
        } else if (gameTile.isDownloadable()) {
            _log.debug("entitlement is downloadable");
            return false;
        } else if (gameTile.isPreparing()) {
            _log.debug("entitlement is preparing");
            return false;
        } else {
            //The download is probably already finished
            _log.debug("entitlement has successfully started downloading");
            return true;
        }
    }

    /**
     * Start downloading the entitlement, assuming the 'Game Library' page has been opened.
     *
     * @param driver  Selenium WebDriver
     * @param offerID The offerID of the entitlement to download
     * @param options {@link DownloadOptions)}. For detailed description see
     *                {@link @startDownloadEntitlement(WebDriver, String, DownloadOptions, List<String>}
     * @return true if the download is started, false otherwise
     * @throws AWTException
     */
    public static boolean startDownloadingEntitlement(WebDriver driver, String offerID, DownloadOptions options) throws AWTException {
        return startDownloadingEntitlement(driver, offerID, options, null);
    }

    /**
     * Start downloading an entitlement leaving the desktop and start menu
     * shortcuts on, and turning off extra content.
     *
     * @param driver  Selenium WebDriver
     * @param offerID The offerID to download
     * @return true if the download is started, false otherwise
     * @throws AWTException
     */
    public static boolean startDownloadingEntitlement(WebDriver driver, String offerID) throws AWTException {
        return startDownloadingEntitlement(driver, offerID, new DownloadOptions());
    }

    /**
     * Finish downloading the full entitlement after it has started, assuming
     * the 'Game Library' page has been opened.
     *
     * @param driver     Selenium WebDriver
     * @param entOfferID The offerID of the entitlement to download
     * @return true if the entitlement successfully completes the download and
     * installation, false otherwise
     */
    public static boolean finishDownloadingEntitlement(WebDriver driver, String entOfferID) {

        GameTile gameTile = new GameTile(driver, entOfferID);

        boolean successfulDownload = false;
        final long timeToWait = 3600000; // 1 hour
        final int installWait = 600; // 10 min
        long startTime = System.currentTimeMillis();
        long totalTime = 0;
        boolean installStartedManually = false;
        while (totalTime < timeToWait && !successfulDownload) {

            if (gameTile.isReadyToPlay() && !gameTile.isDownloading()) {
                successfulDownload = true;
            } else if (gameTile.isReadyToInstall()) {
                //Sometimes an entitlement fails to do anything when we start
                //the installation, so this is an extra check that will stop
                //us from getting stuck in an infinite loop where we keep trying
                //to install a game that just will not install
                if (installStartedManually) {
                    _log.debug("The entitlement is still ready to install despite already having started the installation");
                    break;
                }
                gameTile.install();
                installStartedManually = true;
            } else if (gameTile.isInstalling()) {

                //Non-DiP Small will have a setup.exe process that needs to
                //be killed in order to install properly
                if (EntitlementInfoConstants.NON_DIP_SMALL_OFFER_ID.equals(entOfferID)) {
                    ProcessUtil.killProcess(OriginClient.getInstance(driver), OriginClientData.DEFAULT_GAME_INSTALLER);
                }

                //Wait for install to finish, or give up if it takes too long
                if (!gameTile.waitForReadyToPlay(installWait)) {
                    _log.debug("Timed out waiting for entitlement to finish installing");
                    break;
                }
            }
            Waits.sleep(2000);
            totalTime = System.currentTimeMillis() - startTime;
        }
        return successfulDownload;
    }

    /**
     * Check if Selected DLCs have been completed to download. Prior this function, startDownloadingEntitlement with selectedDLC
     * and finishDownloadingEntitlement have to be called.
     *
     * @param driver     Selenium WebDriver
     * @param dlcList    The list of offerID of the DLCs to download
     * @return true if selected DLCs successfully complete the download and installation, false otherwise
     * @throws AWTException
     */
    public static boolean finishDownloadingSelectedDLC(WebDriver driver, List<String> dlcList) throws AWTException {
        List<DownloadQueueTile> downloadQueueTiles = new ArrayList<DownloadQueueTile>();
        for(int i = 0; i < dlcList.size(); i++) {
            downloadQueueTiles.add(new DownloadQueueTile(driver, dlcList.get(i)));
        }
        final long timeToWait = 1200000; // 20 minutes
        long startTime = System.currentTimeMillis();
        long totalTime = 0;
        while (totalTime < timeToWait) {
            while(!downloadQueueTiles.isEmpty() && downloadQueueTiles.get(0).isGameDownloadComplete()) {
                downloadQueueTiles.remove(0);
            }
            if(downloadQueueTiles.isEmpty()) {
                return true;
            }
            Waits.sleep(2000);
            totalTime = System.currentTimeMillis() - startTime;
        }
        return false;
    }

    /**
     * Handle download dialogs including: language selection, EULA, download
     * info, warning and error dialogs.
     *
     * @param driver                        Selenium WebDriver for the client
     * @param offerId                       the offer ID to download
     * @param options                       {@link DownloadOptions}<br>
     *                                      UI options:<br>
     *                                      - desktopShortcut if true, check the 'Desktop Shortcut' check box. If
     *                                      false, un-check<br>
     *                                      - startMenuShortcut if true, check the 'Start Menu Shortcut' check box.
     *                                      If false, un-check<br>
     *                                      - uncheckExtraContent if true, un-check the 'Extra Content' check boxes
     *                                      (can use UncheckExtraContentExceptions parameter to check some back). If
     *                                      false, do not un-check (and the UncheckExtraContentExceptions parameter
     *                                      is ignored)<br>
     *                                      Flow options:<br>
     *                                      - installFromDisc if true, we are installing from (ITO) disc and a
     *                                      download preparation dialog should have already been opened. If false,
     *                                      initiate download from the game tile's context menu <br>
     *                                      - stopAtEULA if true, stop downlaod when the EULA dialog appears. If
     *                                      false, continue to end of install<br>
     * @param UncheckExtraContentExceptions if options specify all extra
     *                                      contents to be unchecked, this list of offer-ids will then be checked
     *                                      back. Useful when only a subset (often one) of the extra contents should
     *                                      be checked
     * @return true if the download preparation dialogs were successfully
     * handled. If stopAtEula is true, then this will return true once the EULA
     * dialog is open, or false if is never opened. If stopAtEula is false, then
     * this will return true once all the download preparation dialogs have been
     * handled and are closed, or false if something goes wrong
     * @throws AWTException
     */
    public static boolean handleDialogs(WebDriver driver, String offerId,
                                        DownloadOptions options, List<String> UncheckExtraContentExceptions)
            throws AWTException {

        GameTile gameTile = new GameTile(driver, offerId);
        if (!gameTile.waitForPreparing()) {
            _log.debug("Never reached the preparing phase, trying to download anyway");
        }
        String entitlementName = gameTile.getTitle();

        DownloadInfoDialog downloadInfoDialog = new DownloadInfoDialog(driver);
        DownloadGameMsgBox downloadGameMsgBox = new DownloadGameMsgBox(driver);
        DownloadLanguageSelectionDialog langSelectDialog = new DownloadLanguageSelectionDialog(driver);
        DownloadEULASummaryDialog eulaDialog = new DownloadEULASummaryDialog(driver);
        NetworkProblemDialog networkProblemDialog = new NetworkProblemDialog(driver);
        DownloadProblemDialog downloadProblemDialog = new DownloadProblemDialog(driver);
        PunkbusterWarningDialog punkbusterWarningDialog = new PunkbusterWarningDialog(driver);

        boolean dialogsOpened;
        boolean expectedDialogsFound = false;
        boolean languageDialogAppeared = false;
        boolean downloadDialogAppeared = false;
        int retryCounter = 0;
        final int maxRetries = 7;
        do {
            _log.debug("Looping through all download preparation dialogs ...");
            dialogsOpened = false;
            // Skip language dialog check if already processed
            if (!languageDialogAppeared && langSelectDialog.isOpen()) {
                _log.debug("'Language' dialog opens");
                langSelectDialog.clickAccept();
                dialogsOpened = expectedDialogsFound = languageDialogAppeared = true;
            }
            // There can potentially be multiple EULA dialogs
            if (eulaDialog.isOpen()) {
                _log.debug("'Eula' dialog opens");
                if (options.stopAtEULA) {
                    return true;
                }
                eulaDialog.setLicenseAcceptCheckbox(true);
                eulaDialog.clickNextButton();
                dialogsOpened = expectedDialogsFound = true;
            }

            // Skip download info dialog / download game message box check if already processed
            if (!downloadDialogAppeared) {
                if (options.downloadTrigger.equals(DownloadTrigger.INSTALL_FROM_DISC)) {
                    if (downloadGameMsgBox.verifyVisible(entitlementName)) {
                        _log.debug("Installing from Disc through the 'Download Game' message box");
                        downloadGameMsgBox.setDesktopShortcutCheckbox(options.desktopShortcut);
                        downloadGameMsgBox.setStartMenuShortcutCheckbox(options.startMenuShortcut);
                        downloadGameMsgBox.clickNextButton();
                        dialogsOpened = expectedDialogsFound = downloadDialogAppeared = true;
                    }
                } else if (downloadInfoDialog.isOpen()) {
                    _log.debug("'Download info' dialog opens");
                    if (options.uncheckExtraContent) {
                        downloadInfoDialog.uncheckAllExtraContent();
                        if (UncheckExtraContentExceptions != null) {
                            for (String extraContentOfferId : UncheckExtraContentExceptions) {
                                downloadInfoDialog.setExtraContentCheckbox(extraContentOfferId, true);
                            }
                        }
                    }
                    if(options.newInstallLocation != "") {
                        _log.debug(String.format("New install location needs to be set to %s", options.newInstallLocation));
                        if(downloadInfoDialog.setInstallLocation(options.newInstallLocation)) {
                            _log.debug("Successfully changed install location");
                        } else {
                            _log.debug("Failed to change install location");
                            return false;
                        }
                    }
                    downloadInfoDialog.setDesktopShortcut(options.desktopShortcut);
                    downloadInfoDialog.setStartMenuShortcut(options.startMenuShortcut);
                    if (downloadInfoDialog.isPunkBusterPresent()) {
                        downloadInfoDialog.checkPunkBuster();
                    }
                    downloadInfoDialog.clickNext();
                    dialogsOpened = expectedDialogsFound = downloadDialogAppeared = true;
                } else if (downloadProblemDialog.isOpen()) {
                    _log.error("'Download Problem' dialog appears, preparation dialog handling failed");
                    return false;
                }
            }
            if (punkbusterWarningDialog.isOpen()) {
                _log.debug("'Punkbuster Warning' dialog opens");
                punkbusterWarningDialog.clickYes();
                dialogsOpened = expectedDialogsFound = true;
            }

            //Sets a retry flag if no dialogs opened during this iteration
            if (!dialogsOpened) {
                _log.debug("No download preparation dialogs appeared. Incrementing retry counter");
                ++retryCounter;
            }
            Waits.sleep(500); // Pause for old dialog to close and new dialog to appear before re-trying
        } while (gameTile.isPreparing() && retryCounter <= maxRetries);

        //We were attempting to stop at the EULA dialog, but the EULA
        //dialog never appeared
        if (options.stopAtEULA) {
            _log.error("The EULA dialog did not appear");
            return false;
        }

        if (!expectedDialogsFound) {
            _log.error("No download preparation dialogs appeared");
            return false;
        }

        if (networkProblemDialog.isOpen()) {
            _log.debug("'Network Problem' dialog appears, preparation dialog handling failed");
            return false;
        }

        if (gameTile.isPreparing()) {
            _log.debug("The game is still preparing to download, preparation dialog handling failed");
            return false;
        }

        return true;
    }

    /**
     * Handle download dialogs. For more description, see called method.
     * {@link  #handleDialogs(WebDriver, String, DownloadOptions, List<String>)}
     *
     * @param driver  Selenium WebDriver
     * @param offerId the offer ID to download
     * @param options {@link DownloadOptions)}. For detailed description see
     *                {@link @startDownloadEntitlement(WebDriver, String, DownloadOptions, List<String>}
     * @return true if the download preparation dialogs were successfully
     * handled. If stopAtEula is true, then this will return true once the EULA
     * dialog is open, or false if is never opened. If stopAtEula is false, then
     * this will return true once all the download preparation dialogs have been
     * handled and are closed, or false if something goes wrong
     * @throws AWTException
     */
    public static boolean handleDialogs(WebDriver driver, String offerId, DownloadOptions options) throws AWTException {
        return handleDialogs(driver, offerId, options, null);
    }
    
    /**
     * Handles the dialog that can appear when a game is launched.
     *
     * @param driver Selenium WebDriver
     */
    public static void handlePlayDialogs(WebDriver driver) {
        UpdateAvailableDialog updateDialog = new UpdateAvailableDialog(driver);
        if (updateDialog.waitIsVisible()) {
            updateDialog.clickPlayWithoutUpdatingButton();
        }

        CloudSaveConflictDialog saveDialog = new CloudSaveConflictDialog(driver);
        if (saveDialog.waitIsVisible()) {
            saveDialog.clickUseLocalDataButton();
        }
    }

    /**
     * Remove an entitlement from the 'Game Library' using the right-click menu.
     *
     * @param driver Selenium WebDriver
     * @param offerID The offerID of the entitlement to remove
     * @return true if the entitlement is removed, false otherwise
     */
    public static boolean removeFromLibrary(WebDriver driver, String offerID) {
        GameTile gameTile = new GameTile(driver, offerID);
        gameTile.removeFromLibrary();

        RemoveFromLibrary removeFromLibraryDialog = new RemoveFromLibrary(driver);
        removeFromLibraryDialog.waitForVisible();
        removeFromLibraryDialog.confirmRemoval();

        GameLibrary gameLib = new GameLibrary(driver);
        return Waits.pollingWait(() -> !gameLib.isGameTileVisibleByOfferID(offerID, 1));
    }

    /**
     * Verify game exists in the 'Game Library'.
     *
     * @param driver Selenium WebDriver
     * @param entitlementName The name of the entitlement to check
     * @return true if it exists, false otherwise
     */
    public static boolean verifyGameInLibrary(WebDriver driver, String entitlementName) {
        NavigationSidebar navBar = new NavigationSidebar(driver);
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        return Waits.pollingWait(() -> gameLibrary.isGameTileVisibleByName(entitlementName));
    }

    /**
     * To upgrade an entitlement from a lower edition to the next higher edition
     * by navigating to the 'Game Library' and upgrade through the slide out.
     *
     * @param driver                     Selenium WebDriver
     * @param entitlementOfferId         The entitlement to be upgraded
     * @param upgradedEntitlementOfferId The offerID of the entitlement to be
     *                                   upgraded to
     * @return true if the entitlement is upgraded and the upgraded version is
     * added to the game library
     */
    public static boolean upgradeEntitlement(WebDriver driver, String entitlementOfferId, String upgradedEntitlementOfferId) {
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();

        if (!gameLibrary.isGameTileVisibleByOfferID(entitlementOfferId)) {
            _log.debug("Error: Could not find the entitlement " + entitlementOfferId + " in the game library");
            return false;
        }

        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        GameSlideout gameSlideout = gameTile.openGameSlideout();
        gameSlideout.upgradeEntitlement();
        CheckoutConfirmation vaultConfirmationDialog = new CheckoutConfirmation(driver);

        if (!vaultConfirmationDialog.waitIsVisible()) {
            _log.debug("Error: Checkout confirmation dialog for the entitlement " + entitlementOfferId + " was not open");
            return false;
        }

        vaultConfirmationDialog.clickCloseCircle();

        if (!gameLibrary.isGameTileVisibleByOfferID(upgradedEntitlementOfferId)) {
            _log.debug("Error: Could not upgrade the entitlement to " + upgradedEntitlementOfferId + " in the game library");
            return false;
        }

        return true;
    }

    /**
     * Add the given 'Demos And Betas' tile to the 'Game Library' by clicking the
     * corresponding 'Get It Now' button assuming the user is already on the 'Demos
     * and Betas' page
     *
     * @param offerId The offerId of the entitlement to be selected to add to
     *                the 'Game Library'
     */
    public static void addDemoGameToLibrary(WebDriver driver, String offerId) {
        DemoTile demoTile = new DemosAndBetasPage(driver).getDemoAndBetaTile(offerId);
        demoTile.hoverOnGetItNowButton();
        demoTile.clickGetItNowButton();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitForVisible();
        checkoutConfirmation.clickCloseCircle();
    }

    /**
     * Launch a game and exit the game after a few seconds of the game running.
     *
     * @param driver          {@link WebDriver}
     * @param entitlementInfo The entitlement to launch
     * @param client          The client from which the game has to be launched/exited
     * @return true if successfully launched and exited the given entitlement, false otherwise
     */
    public static boolean launchAndExitGame(WebDriver driver, EntitlementInfo entitlementInfo, OriginClient client) {
        try {
            GameTile gameTile = new GameTile(driver, entitlementInfo.getOfferId());
            gameTile.play();

            boolean isLaunched = entitlementInfo.isLaunched(client);

            if (isLaunched) {
                Waits.sleep(30000); // wait for 30 secs to launch and play
                boolean isKilledGameProcess = entitlementInfo.killLaunchedProcess(client);
                if (isKilledGameProcess) {
                    return Waits.pollingWait(() -> gameTile.isReadyToPlay());
                } else {
                    _log.error("Failed at killing entitlement: '" + entitlementInfo.getName() + "'");
                }
            } else {
                _log.error("Failed at launching entitlement: '" + entitlementInfo.getName() + "'");
            }
        } catch (IOException | InterruptedException ex) {
            _log.error("error..... " + ex.getMessage());
            return false;
        }
        return false;
    }

    /**
     * Launch a game with JS-SDK and exit the game non-forcibly after a few seconds of the game running.
     *
     * This method is created for memory profile test case.
     *
     * @param driver          {@link WebDriver}
     * @param entitlementInfo The entitlement to launch
     * @param client          The client from which the game has to be launched/exited
     * @return true if successfully launched and exited the given entitlement, false otherwise
     */
    public static boolean launchByJsSDKAndExitGame(WebDriver driver, EntitlementInfo entitlementInfo, OriginClient client) {
        try {
            GameTile gameTile = new GameTile(driver, entitlementInfo.getOfferId());
            gameTile.playByJSSDK(entitlementInfo.getOfferId());
            boolean isLaunched = entitlementInfo.isLaunched(client);
            if (isLaunched) {
                boolean isKilledGameProcess = ProcessUtil.killProcessWithoutForce(client,
                        entitlementInfo.getProcessName());

                if (isKilledGameProcess) {
                    return Waits.pollingWait(() -> gameTile.isReadyToPlay());
                } else {
                    _log.error("Failed at killing entitlement: '" + entitlementInfo.getName() + "'");
                }
            } else {
                _log.error("Failed at launching entitlement: '" + entitlementInfo.getName() + "'");
            }
        } catch (IOException | InterruptedException ex) {
            _log.error("error..... " + ex.getMessage());
            return false;
        }
        return false;
    }

    /**
     * Launch a game and exit the game after few seconds of the game running
     * and repeat these for x number of times.
     * This test is basically used to play the game and exit
     * inorder to test cloud saves for an entitlement.
     *
     * @param entitlementInfo The entitlement to launch
     * @param client          The client from which the game has to be launched/exited
     * @return true if successfully launched and exited the given entitlement x number of times, false otherwise
     */
    public static boolean launchAndExitGameForXTimes(WebDriver driver, EntitlementInfo entitlementInfo, OriginClient client, int countValue) {
       for (int i = 1; i <= countValue; i++) {
            new NavigationSidebar(driver).gotoGameLibrary();
           if(!launchAndExitGame(driver, entitlementInfo, client)){
               return false;
           }
        }
        return true;
    }

    /**
     * Silently uninstall entitlement given a set of guids before installation and a set of guids after installation. Checks to see if there is only
     * one new guid, else throw exception.
     *
     * @param guidsBefore Set of guids before installation
     * @param guidsAfter Set of guids after installation
     * @param driver Selenium WebDriver
     * @param entitlement Entitlement to uninstall
     * @param client The Origin client
     *
     * @throws Exception when there is more than 1 new guid or when there is no new guid
     */
    public static void silentUninstallEntitlementGivenGuid(Set<String> guidsBefore, Set<String> guidsAfter, WebDriver driver, EntitlementInfo entitlement, OriginClient client) throws Exception {
        guidsAfter.removeAll(guidsBefore);
        if (!guidsAfter.isEmpty() && guidsAfter.size() == 1) {
            entitlement.enableSilentUninstall(client, StringHelper.normalizeSetString(guidsAfter.iterator().next()));
            Waits.sleep(3000); // sleep because sometimes 'uninstall' may not be in context menu right away
            new GameTile(driver, entitlement.getOfferId()).uninstallForLocale();
        } else {
            throw new Exception("There is more than 1 new guid or there is no new guid.");
        }
    }

    /**
     * Verify if Download Manager, Download Queue Progress bars and percentage shown are similar
     *
     * @param downloadManager the download manager to reference to
     * @param downloadQueueTile the download queue to reference to
     * @return if Download Manager progress bar, Download Queue progress bar and percentage are similar
     */
    public static boolean verifyProgressBarSimilar(DownloadManager downloadManager, DownloadQueueTile downloadQueueTile) {
        Waits.sleep(15000); // Wait client to download some percentage of the file to be able to compare
        int managerProgressBarValue = downloadManager.getProgressBarValue();
        int queueTilePercentageValue = downloadQueueTile.getDownloadPercentageValue();
        int queueTileProgressBarValue = downloadQueueTile.getProgressBarPercentageValue();

        return TestScriptHelper.approxEqual(managerProgressBarValue, queueTilePercentageValue)
                && TestScriptHelper.approxEqual(managerProgressBarValue, queueTileProgressBarValue);
    }


    /**
     * Verify if the DownloadManager window is similar to download manager tile
     * It checks three times because its possible that the frame between each check is different and jumps to the next
     * one right away. If one of the checks is true, then its possible that the frames between the two checks are
     * similar.
     *
     * @param downloadManager The download manager to reference
     * @param downloadQueueTile the download tile to reference
     * @return true if download queue and download queue tile remaining time is similar
     * @throws ParseException when parsing of time fails
     */
    public static boolean verifyDownloadTileAndQueueTileTimeSimilar(DownloadManager downloadManager, DownloadQueueTile downloadQueueTile) throws ParseException {
        String managerTimeRemainingString1 = downloadManager.getETAText();
        String queueTileTimeRemainingString1 = downloadQueueTile.getETAText();
        String managerTimeRemainingString2 = downloadManager.getETAText();
        String queueTileTimeRemainingString2 = downloadQueueTile.getETAText();

        int managerTimeRemaining1 = TestScriptHelper.getSeconds(managerTimeRemainingString1);
        int queueTileTimeRemaining1 = TestScriptHelper.getSeconds(queueTileTimeRemainingString1);
        int managerTimeRemaining2 = TestScriptHelper.getSeconds(managerTimeRemainingString2);
        int queueTileTimeRemaining2 = TestScriptHelper.getSeconds(queueTileTimeRemainingString2);

        boolean similarTimeRemaining1 = managerTimeRemaining1 == queueTileTimeRemaining1;
        boolean similarTimeRemaining2 = queueTileTimeRemaining1 == managerTimeRemaining2;
        boolean similarTimeRemaining3 = managerTimeRemaining2 == queueTileTimeRemaining2;

        _log.debug("Download Manager time 1: " + managerTimeRemaining1 + " Queue tile time 1:" + queueTileTimeRemaining1 + " Download Manager time 2:" + managerTimeRemaining2 + " Queue tile time 2:" + queueTileTimeRemaining2);

        return similarTimeRemaining1 || similarTimeRemaining2 || similarTimeRemaining3;
    }
}