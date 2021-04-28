package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.dialog.CheckoutConfirmation;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import java.lang.invoke.MethodHandles;
import java.util.Arrays;

/**
 * Macro class containing static methods for multi-step actions related to the GDP.
 *
 * @author nvarthakavi
 */
public final class MacroGDP {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());
    private static final String EXPECTED_TITLE = " for PC | Origin";


    /**
     * Constructor
     */
    private MacroGDP() {
    }

    /**
     * Navigates to the GDP of a game by searching for the title in the 'Browse
     * Games' page.
     *
     * @param driver          Selenium WebDriver
     * @param entitlementInfo Entitlement to navigate to
     * @return true if successfully navigated to the entitlement's GDP page from the
     * 'Browse Games' page, false otherwise
     */
    public static boolean loadGdpPageBySearch(WebDriver driver, EntitlementInfo entitlementInfo) {
        return loadGdpPageBySearch(driver, entitlementInfo.getName());
    }

    /**
     * Navigates to the GDP of a game by searching for the title in the 'Browse
     * Games' page.
     *
     * @param driver          Selenium WebDriver
     * @param entitlementName Name of the game to navigate to
     * @return true if successfully navigated to the entitlement's GDP page from the
     * 'Browse Games' page, false otherwise
     */
    public static boolean loadGdpPageBySearch(WebDriver driver, String entitlementName) {
        //Sometimes long strings will only be entered partially, so a retry will need to be done
        int retry = 2;
        GlobalSearch globalSearch = new GlobalSearch(driver);
        boolean verifiedSearchBoxValue = false;
        do {
            globalSearch.enterSearchText(entitlementName.replaceAll("™", "").replaceAll("®", ""));
        }
        while (!(verifiedSearchBoxValue = globalSearch.verifySearchBoxValue(entitlementName.replaceAll("™", "").replaceAll("®", ""))) && --retry > 0);

        if (!verifiedSearchBoxValue) {
            _log.error("Unable to enter search term: " + entitlementName);
            return false;
        }

        //If game shows up within initial store search, go to PDP.
        //Else click view all store search results.
        GlobalSearchResults globalSearchResults = new GlobalSearchResults(driver);
        globalSearchResults.waitForResults();
        globalSearchResults.waitForGamesToLoad(); //Waits for the game titles to load before checking for their visibility
        if (globalSearchResults.verifyStoreResultContainsOffer(entitlementName)) {
            globalSearchResults.viewGDPOfGame(entitlementName);
            return verifyGdpHeaderLoad(driver, entitlementName);
        }

        //If there is no view all, return false
        if (!globalSearchResults.verifyViewAllStoreResults()) {
            _log.error("Cannot find 'View All' link on store search results");
            return false;
        }

        //Check if game tile is visible in view all search
        globalSearchResults.clickViewAllStoreResults();
        globalSearchResults.waitForResults();
        globalSearchResults.waitForGamesToLoad(); // wait for games to load before checking visibility
        if (!globalSearchResults.verifyStoreResultContainsOffer(entitlementName)) {
            _log.error("Cannot find " + entitlementName + " in view all of store results");
            return false;
        }

        globalSearchResults.viewGDPOfGame(entitlementName);
        return verifyGdpHeaderLoad(driver, entitlementName);
    }

    /**
     * Verify the correct GDP hero is loaded.
     *
     * @param driver          Selenium WebDriver
     * @param entitlementName Name of the game to navigate to
     * @return true if the correct GDP header is loaded, false otherwise
     */
    private static boolean verifyGdpHeaderLoad(WebDriver driver, String entitlementName) {
        GDPHeader gdpHeader = new GDPHeader(driver);
        return gdpHeader.verifyGameTitle(entitlementName);
    }

    /**
     * Loads the GDP page directly going directly to the GDP URL
     *
     * @param driver        Selenium WebDriver
     * @param entName       The parent name of the entitlement
     * @param partialGdpUrl The Partial GDP URL of the entitlement, used for
     *                      opening a GDP page on the browser
     * @return true if the GDP page loaded up successfully, false otherwise
     */
    public static boolean loadGdpPage(WebDriver driver, String entName, String partialGdpUrl) {
        GDPHeader gdpHeader = new GDPHeader(driver);
        boolean result = loadPartialGDPURL(driver, partialGdpUrl) &&
                Waits.pollingWait(() -> gdpHeader.verifyGameTitle(entName) &&
                        Waits.pollingWait(() -> new GDPActionCTA(driver).verifyPrimaryCTAVisible()));

        Waits.sleep(1000); // needed to add wait because clicking the button too fast after the page loads causes checkout to get stuck
        return result;
    }

    /**
     * Loads the GDP page directly by using the given partial GDP URL.
     *
     * @param driver        Selenium WebDriver
     * @param partialGdpUrl The Partial GDP URL of the entitlement, used for
     *                      opening a GDP page on the browser
     * @return true if the GDP page loaded up successfully, false otherwise
     */
    public static boolean loadPartialGDPURL(WebDriver driver, String partialGdpUrl) {
        if (partialGdpUrl.isEmpty()) {
            String errorMessage = String.format("Given partial GDP URL is undefined.");
            _log.error(errorMessage);
            throw new RuntimeException(errorMessage);
        }

        //eg. split https://qa.www.origin.com/myhome?automation=true&& using ? as delimiter
        String url[] = driver.getCurrentUrl().split("\\?");
        // split the first part of url https//... using '/' as delimiter
        String partialURL[] = url[0].split("\\/");
        // Copy elements https,'',qa.www.origin.com(,'can','en-us') and join them intro a string using '/'
        String partialURLAfterReplace = String.join("/", Arrays.copyOfRange(partialURL, 0, 5));
        // final url is concatenated to this url https//qa.www.origin.com/store/battlefield4/....?automation=true&..&
        // partial GDP url's contain a ?, so we should not add another if there is already one there.
        String gdpPageUrl = partialURLAfterReplace + partialGdpUrl.replace("?", "") + '?' + url[1];

        driver.get(gdpPageUrl);
        GDPHeader gdpHeader = new GDPHeader(driver);
        boolean resultOK = gdpHeader.verifyGDPHeaderReached();
        if (!resultOK) {
            _log.error("Failed to load GDP page using given partial GDP URL.");
        }

        return resultOK;
    }

    /**
     * Loads the GDP Page of a given entitlement directly.
     *
     * @param driver          Selenium WebDriver
     * @param entitlementInfo The entitlementInfo object to extract the
     *                        information from
     * @return true if the GDP page loaded up successfully, false otherwise
     */
    public static boolean loadGdpPage(WebDriver driver, EntitlementInfo entitlementInfo) {
        return loadGdpPage(driver, entitlementInfo.getParentName(), entitlementInfo.getPartialPdpUrl());
    }

    /**
     * Add a vault game to library by clicking 'AddToLibrary' CTA, close the
     * checkout confirmation dialog and verify 'ViewInLibrary' CTA is present
     *
     * @param driver Selenium WebDriver
     * @return true if 'ViewInLibrary' CTA is present, false otherwise
     */
    public static boolean addVaultGameAndCheckViewInLibraryCTAPresent(WebDriver driver) {
        GDPActionCTA gdpActionCTA = new GDPActionCTA(driver);
        gdpActionCTA.clickAddToLibraryVaultGameCTA();
        CheckoutConfirmation checkoutConfirmation = new CheckoutConfirmation(driver);
        checkoutConfirmation.waitIsVisible();
        checkoutConfirmation.clickCloseCircle();
        return gdpActionCTA.verifyViewInLibraryCTAPresent();
    }

    /**
     * Navigates to a specific game, adds vault game to library by clicking
     * 'AddToLibrary' CTA, close the checkout confirmation dialog and verify
     * 'ViewInLibrary' CTA is present
     *
     * @param driver Selenium WebDriver
     * @param entitlement entitlement to add in library
     * @return true if 'ViewInLibrary' CTA is present, false otherwise
     */
    public static boolean addVaultGameAndCheckViewInLibraryCTAPresent(WebDriver driver, EntitlementInfo entitlement) {
        if (!MacroGDP.loadGdpPage(driver, entitlement.getParentName(), entitlement.getPartialPdpUrl())) {
            _log.error(String.format("Cannot open GDP page for: %s", entitlement.getParentName()));
            return false;
        }

        return addVaultGameAndCheckViewInLibraryCTAPresent(driver);
    }

    /**
     * Returns the expected page title for GDPs
     *
     * @return String expected title
     */
    public static String getExpectedGDPTitle(EntitlementInfo entitlementInfo) {
        return entitlementInfo.getName() + EXPECTED_TITLE;
    }
}