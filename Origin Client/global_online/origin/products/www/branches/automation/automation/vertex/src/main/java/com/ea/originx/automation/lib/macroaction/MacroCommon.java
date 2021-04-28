package com.ea.originx.automation.lib.macroaction;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.common.GlobalFooter;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.NotificationCenter;
import com.ea.originx.automation.lib.pageobjects.dialog.LanguagePreferencesDialog;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.utils.Waits;
import static java.lang.Thread.sleep;
import java.lang.invoke.MethodHandles;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Macro class containing static methods for multi-step actions relating to 'Common' page objects.
 *
 * @author palui
 */
public final class MacroCommon {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Private constructor
     */
    private MacroCommon() {
    }

    /**
     * Checks the Origin URL language code and the Global Footer current language to
     * determine if Origin at browser (logged in or out) is displaying in the
     * specified language.
     *
     * @param driver Selenium WebDriver
     * @param languageInfo Language to match
     *
     * @return true if browser is in the specified language, false otherwise
     */
    public static boolean verifyBrowserLanguage(WebDriver driver, LanguageInfo languageInfo) {
        // Checks the Origin URL language code and the Global Footer
        boolean urlLanguageOK = languageInfo.verifyOriginUrlLanguage(driver.getCurrentUrl());
        boolean globalFooterLanguageOK = new GlobalFooter(driver).verifyCurrentLanguage(languageInfo);
        return urlLanguageOK && globalFooterLanguageOK;
    }

    /**
     * Opens the 'LanguagePreferences' dialog for Origin on the browser. Works
     * whether user is logged in or not.
     *
     * @param driver Selenium WebDriver
     * @return 'Language Preferences' dialog instance, or throw exception if it
     * cannot be opened
     */
    public static LanguagePreferencesDialog openLanguagePreferencesDialog(WebDriver driver) {
        LanguagePreferencesDialog lpDialog = new LanguagePreferencesDialog(driver);
        if (!lpDialog.isOpen()) {
            NavigationSidebar navSidebar = new NavigationSidebar(driver);
            if (navSidebar.verifyLoggedOutLanguagePreferencesIconVisible()) { // logged out
                navSidebar.clickLoggedOutLanguagePreferencesIcon();
            } else { // logged in
                MiniProfile miniProfile = new MiniProfile(driver);
                miniProfile.waitForPageToLoad();
                miniProfile.selectBrowserLanguagePreferences();
            }
            if (!lpDialog.waitIsVisible()) {
                throw new RuntimeException("Cannot open 'Language Preferences' dialog");
            }
        }
        return lpDialog;
    }

    /**
     * Sets the language for Origin at the browser
     *
     * @param driver Selenium WebDriver
     * @param languageInfo The language to set the browser language to
     *
     * @return true if language was set successfully, false otherwise.
     */
    public static boolean setBrowserLanguage(WebDriver driver, LanguageInfo languageInfo) {
        openLanguagePreferencesDialog(driver).setLanguageAndSave(languageInfo);
        return verifyBrowserLanguage(driver, languageInfo);
    }

    /**
     * This checks if any of words on last segment of current URL matches with
     * any of words on last segment of URL in href tag.
     *
     * @param driver Selenium WebDriver
     * @return brokenLinkList if any broken link found, return empty list if not
     * found
     */
    public static List<String> verifyAllHyperLinksInPage(WebDriver driver) {

        List<String> brokenLinkList = new ArrayList<>();
        List<WebElement> elements = driver.findElements(By.cssSelector("#content a[href]"));

        if (elements.isEmpty()) {
            throw new RuntimeException("error: there is no link in content page");
        }

        // make list of unique link URL in page
        List<String> hyperlinks = elements.stream()
                .filter(element -> StringHelper.containsAnyIgnoreCase(element.getAttribute("href"), "http"))
                .map(element -> element.getAttribute("href"))
                .distinct()
                .collect(Collectors.toList());

        // if URL ends with "/", then remove "/" for comparing words from last segment of two URL
        for (int i = 0; i < hyperlinks.size(); i++) {
            String link = hyperlinks.get(i);
            if (link.endsWith("/")) {
                link = link.substring(0, link.length() - 1);
                hyperlinks.set(i, link);
            }
        }

        // this will get URL as key and 'demos', 'and', 'betas' as values from URL below as example
        // https://qa.www.origin.com/can/en-us/store/free-games/demos-and-betas
        Map<String, String[]> maps = hyperlinks.stream()
                .collect(Collectors.toMap(p -> p, p -> (p.split("/")[(p.split("/").length) - 1].split("-"))));

        // open all link saved in the list and get words from last segment of URL opened
        // need to wait for page loading due to redirection(s)
        String urlOpened;
        for (Map.Entry<String, String[]> entry : maps.entrySet()) {
            int matchCount = 0;
            boolean foundMatchedURL = false;
            driver.get(entry.getKey());
            Waits.pageWait(driver, 60);
            urlOpened = driver.getCurrentUrl();
            if (urlOpened.endsWith("/")) {
                urlOpened = urlOpened.substring(0, urlOpened.length() - 1);
            }
            String[] lastPathStrings = urlOpened.split("/")[(driver.getCurrentUrl().split("/")).length - 1].split("-");

            for (String value : entry.getValue()) {
                for (String lastPathString : lastPathStrings) {
                    if (StringHelper.containsAnyIgnoreCase(value, lastPathString)) {
                        matchCount++;
                        break;
                    }
                }

                // if one word from both URL is same in case one of them has only one word, then it is considered as not broken link
                if ((lastPathStrings.length == 1 || entry.getValue().length == 1) && matchCount >= 1) {
                    _log.debug("debug: one word is matched from last segment for current URL :" + urlOpened + " and href tag : " + entry.getKey());
                    foundMatchedURL = true;
                    break;

                    // if two words from both URL are same in case both has two or more words, then it is considered as not broken link
                } else if (lastPathStrings.length >= 2 && entry.getValue().length >= 2 && matchCount >= 2) {
                    _log.debug("debug: two words are matched from last segment for current URL :" + urlOpened + " and href tag : " + entry.getKey());
                    foundMatchedURL = true;
                    break;
                }
            }
            // if no matched word from last segment of two URL
            if (!foundMatchedURL) {
                _log.error("debug: current URL :" + urlOpened + " does not match with URL in href tag : " + entry.getKey());
                brokenLinkList.add(entry.getKey());
            }
        }
        return brokenLinkList;
    }

    /**
     * Add a non-Origin game (an executable) to the 'Game Library'.
     *
     * @param exePath The path to the executable to add to the 'Game Library'
     * @param driver Selenium WebDriver
     * @param client The Origin client
     */
    public static void addNonOriginGame(String exePath, WebDriver driver, OriginClient client) throws Exception {
        new MainMenu(driver).selectAddNonOriginGame();
        sleep(30000); // wait until window stabilized
        RobotKeyboard robotHandler = new RobotKeyboard();
        robotHandler.type(client, exePath, 200);
        sleep(1000); // wait a while until pressing enter
        robotHandler.type(client, "\n\n", 2000);
    }

    /**
     * Open the first gift in the notification center and add it to the 'Game Library'.
     *
     * @param driver Selenium WebDriver
     */
    public static void openGiftAddToLibrary(WebDriver driver) {
        new NotificationCenter(driver).clickYouGotGiftNotification();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        openGiftPage.waitForVisible();
        openGiftPage.clickCloseButton();
    }
}