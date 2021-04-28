package com.ea.originx.automation.scripts.navigation;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroCommon;
import com.ea.originx.automation.lib.pageobjects.dialog.LanguagePreferencesDialog;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;

import java.net.MalformedURLException;
import java.util.ArrayList;
import org.openqa.selenium.JavascriptExecutor;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test 'Discover' link and 'Discover' page disappearing after logout
 *
 * @author palui
 */
public class OABrowserLanguage extends EAXVxTestTemplate {

    /**
     * This method tests browser language persistence when the browser is in
     * incognito mode (requirement for Automation). To preserve browser history,
     * a 2nd window is open in the original browser window before closing
     * Origin. A third Origin browser window is then open with the same Selenium
     * WebDriver to test language persistence. When test completes, the new
     * transient window is closed and driver points to the new Origin window
     *
     * @param driver       Selenium WebDriver
     * @param siteURL      Site URL of Origin to enter on the second Origin window
     * @param languageInfo language to check for persistence
     * @throws MalformedURLException
     */
    public static boolean verifyBrowserLanguagePersistence(WebDriver driver, String siteURL, LanguageInfo languageInfo) throws MalformedURLException {
        // Open transient window to preserve session history in incognito mode, since history is lost if
        // the Origin browser window is the last incognito window to close

        executeJavascriptCommand(driver, "window.open(\"" + "http://www.google.com" + "\",\"\", \"fullscreen=yes,top=0,left=0\");");
        sleep(5000); // pause for transient window to open, otherwsie may not appear in getWindowHandles yet
        ArrayList<String> windows = new ArrayList<>(driver.getWindowHandles());
        String transientWindow = windows.get(1);

        // Close the old Origin window and open a new Origin window to verify language is preserved
        driver.close();
        driver.switchTo().window(transientWindow);
        executeJavascriptCommand(driver, "window.open(\"" + siteURL + "\",\"\", \"fullscreen=yes,top=0,left=0\");");
        sleep(5000); // pause for new Origin window to open, otherwsie it may not appear in getWindowHandles yet
        windows = new ArrayList<>(driver.getWindowHandles());
        String newOriginWindow = windows.get(1);
        driver.switchTo().window(newOriginWindow);
        driver.manage().window().maximize(); // maximize or the nav side bar may not be visible
        boolean persists = MacroCommon.verifyBrowserLanguage(driver, languageInfo);

        // Close the transient window
        driver.switchTo().window(transientWindow);
        driver.close();
        driver.switchTo().window(newOriginWindow);
        return persists;
    }

    @TestRail(caseId = 9513)
    @Test(groups = {"navigation", "browser_only"})
    public void testBrowserLanguage(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        LanguageInfo englishUSInfo = new LanguageInfo(LanguageEnum.ENGLISH);
        String englishUSName = englishUSInfo.getName();
        String englishUSLocalName = englishUSInfo.getLocalName();
        LanguageInfo languageInfo2 = new LanguageInfo(LanguageEnum.FRENCH_CANADA);
        String languageName2 = languageInfo2.getName();
        LanguageInfo languageInfo3 = new LanguageInfo(LanguageEnum.JAPANESE);
        String languageName3 = languageInfo3.getName();

        logFlowPoint(String.format("Open browser and go to 'https://*origin.com*'. Verify Origin comes up in '%s' by default", englishUSName)); //1
        logFlowPoint(String.format("Open 'Language Preferences' dialog. Verify language '%s' is selected by default", englishUSLocalName)); //2
        logFlowPoint(String.format("Set language to '%1$s' and click save. Verify Origin language has changed to '%1$s'", languageName2)); //3
        logFlowPoint(String.format("Close and re-open browser. Go to 'https://*origin.com*'. Verify Origin language remains in '%s'", languageName2)); //4
        logFlowPoint(String.format("Set language to '%1$s' and click save. Verify Origin language has changed to '%1$s'", languageName3)); //5
        logFlowPoint(String.format("Close and re-open browser. Go to 'https://*origin.com*'. Verify Origin language remains in '%s'", languageName3)); //6

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroCommon.verifyBrowserLanguage(driver, englishUSInfo)) {
            logPass(String.format("Verified Origin comes up in '%s' by default", englishUSName));
        } else {
            logFailExit(String.format("Failed: Origin does not come up in '%s' by default", englishUSName));
        }
        String siteURL = TestScriptHelper.getCurrentSiteURL(driver);

        //2
        LanguagePreferencesDialog lpDialog = MacroCommon.openLanguagePreferencesDialog(driver);
        lpDialog.waitIsVisible();
        if (lpDialog.verifySelectedLanguage(englishUSInfo)) {
            logPass(String.format("Verified 'Language Preferences' dialog successfully opens with '%s' selected", englishUSLocalName));
        } else {
            logFailExit(String.format("Failed: 'Language Preferences' dialog does not open or '%s' is not the selected language", englishUSLocalName));
        }

        //3
        lpDialog.setLanguageAndSave(languageInfo2);
        if (MacroCommon.verifyBrowserLanguage(driver, languageInfo2)) {
            logPass(String.format("Verified Origin language has changed to '%s'", languageName2));
        } else {
            logFailExit(String.format("Failed: Origin language has not changed to '%s'", languageName2));
        }

        //4
        if (verifyBrowserLanguagePersistence(driver, siteURL, languageInfo2)) {
            logPass(String.format("Verified closing browser and re-opening Origin does not change the language away from '%s'", languageName2));
        } else {
            logFailExit(String.format("Failed: Closing browser and re-opening Origin changes the language away from '%s'", languageName2));
        }

        //5
        MacroCommon.setBrowserLanguage(driver, languageInfo3);
        if (MacroCommon.verifyBrowserLanguage(driver, languageInfo3)) {
            logPass(String.format("Verified Origin language has changed to '%s'", languageName3));
        } else {
            logFailExit(String.format("Failed: Origin language has not changed to '%s'", languageName3));
        }

        //6
        if (verifyBrowserLanguagePersistence(driver, siteURL, languageInfo3)) {
            logPass(String.format("Verified closing browser and re-opening Origin does not change the language away from '%s'", languageName3));
        } else {
            logFailExit(String.format("Failed: Closing browser and re-opening Origin changes the language away from '%s'", languageName3));
        }
        softAssertAll();
    }
}
