package com.ea.originx.automation.lib.pageobjects.settings;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.resources.LanguageInfo;
import com.ea.vx.originclient.resources.LanguageInfo.LanguageEnum;
import java.lang.invoke.MethodHandles;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.IntStream;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.Keys;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.Select;

/**
 * Page object representing the 'Settings - Application' page.
 *
 * @author palui
 * @author lscholte
 */
public class AppSettings extends SettingsPage {

    private static final String APP_SETTINGS_SECTION = "#application";
    private static final By APP_SETTINGS_HEADER_LOCATOR = By.cssSelector(".origin-settings-header .origin-settings-div .origin-settings-wrap h1.otktitle-2");
    private static final String KEEP_GAMES_UP_TO_DATE_LOCATOR = "div.origin-telemetry-settings-autoupdategames > div";
    private static final String AUTO_UPDATE_ON_CSS_TMPL = KEEP_GAMES_UP_TO_DATE_LOCATOR + ".otktoggle-ison";
    private static final String SHOW_ORIGIN_AFTER_GAMEPLAY_ITEMSTR = "Show Origin after gameplay";

    private static final String CLIENT_UPDATE_SECTION = "#update";
    private static final String KEEP_GAMES_UP_TO_DATE_ITEMSTR = "Automatic game updates";
    private static final String KEEP_ORIGIN_UP_TO_DATE_ITEMSTR = "Automatically update Origin";
    private static final String PARTICIPATE_IN_ORIGIN_CLIENT_BETAS_ITEMSTR = "Participate in Origin client betas";

    private static final String START_UP_OPTIONS_SECTION = "#start-up";
    private static final String AUTOMATICALLY_START_ORIGIN_ITEMSTR = "Automatically start Origin";
    private static final String ORIGIN_HELPER_SERVICE_ITEMSTR = "Origin Helper service";

    private static final By LANGUAGE_DROPDOWN = By.xpath("//DIV[contains(@class, 'origin-settings-div-main') and @itemstr='Language']"
            + "/..//SELECT[contains(@class, 'origin-settings-select')]");

    private static final By DOWNLOAD_RESTRICTION_SELECTION_DROPDOWN = By.xpath("//DIV[contains(@class, 'origin-settings-div-main')]"
            + "/..//SELECT[contains(@class, 'origin-settings-select') and @ng-model='throttleSpeedOutOfGame']");

    // Close button for Application Settings Inside Oig
    protected static final By CLOSE_BUTTON_LOCATOR = By.xpath("//*[@id='buttonContainer']/*[@id='btnClose']");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AppSettings(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Settings - Application' page is displayed.
     *
     * @return true if 'Settings - Application' is displayed, false otherwise
     */
    public boolean verifyAppSettingsReached() {
        return getActiveSettingsPageType() == PageType.APPLICATION;
    }

    /**
     * Get 'Application Settings' header text.
     *
     * @return The 'Application Settings' header text as a String
     */
    public String getAppSettingsHeaderText() {
        return waitForElementVisible(APP_SETTINGS_HEADER_LOCATOR).getText();
    }

    /**
     * Toggle 'Show Origin After Gameplay' in the 'Application Settings' section.
     */
    public void toggleShowOriginAfterGameplay() {
        clickToggleSlide(APP_SETTINGS_SECTION, SHOW_ORIGIN_AFTER_GAMEPLAY_ITEMSTR);
    }

    /**
     * Toggle 'Automatic Game Updates' in the 'Client Update' section.
     */
    public void toggleKeepGamesUpToDate() {
        WebElement toggle = waitForElementClickable(By.cssSelector(KEEP_GAMES_UP_TO_DATE_LOCATOR));
        scrollToElement(toggle);
        toggle.click();
    }

    /**
     * Set 'Automatic Game Updates' in the 'Client Update' section to the
     * specified state.
     *
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off.
     */
    public void setKeepGamesUpToDate(boolean toggleState) {
        if(toggleState != verifyKeepGamesUpToDate(true)) {
            toggleKeepGamesUpToDate();
        }
    }

    /**
     * Toggle 'Automatically Update Origin' in the 'Client Update' section.
     */
    public void toggleKeepOriginUpToDate() {
        clickToggleSlide(CLIENT_UPDATE_SECTION,
                KEEP_ORIGIN_UP_TO_DATE_ITEMSTR);
    }

    /**
     * Set 'Automatically Update Origin' in the 'Client Update' section to the
     * specified state.
     *
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off.
     */
    public void setKeepOriginUpToDate(boolean toggleState) {
        setToggleSlide(CLIENT_UPDATE_SECTION,
                KEEP_ORIGIN_UP_TO_DATE_ITEMSTR,
                toggleState);
    }

    /**
     * Toggle 'Participate in Origin Clients Betas' in the 'Client Update'
     * section.
     */
    public void toggleParticipateInOriginClientsBetas() {
        clickToggleSlide(CLIENT_UPDATE_SECTION,
                PARTICIPATE_IN_ORIGIN_CLIENT_BETAS_ITEMSTR);
    }

    /**
     * Set 'Participate in Origin Clients Betas' in the 'Client Update' section
     * to the specified state.
     *
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off.
     */
    public void setParticipateInOriginClientBetas(boolean toggleState) {
        setToggleSlide(CLIENT_UPDATE_SECTION,
                PARTICIPATE_IN_ORIGIN_CLIENT_BETAS_ITEMSTR,
                toggleState);
    }

    /**
     * Toggle 'Automatically Start Origin' in the 'Start-Up Options' section.
     */
    public void toggleAutomaticallyStartOrigin() {
        clickToggleSlide(START_UP_OPTIONS_SECTION,
                AUTOMATICALLY_START_ORIGIN_ITEMSTR);
    }

    /**
     * Set 'Automatically Start Origin' in the 'Start-Up Options' section to the
     * specified state.
     *
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off.
     */
    public void setAutomaticallyStartOrigin(boolean toggleState) {
        setToggleSlide(START_UP_OPTIONS_SECTION,
                AUTOMATICALLY_START_ORIGIN_ITEMSTR,
                toggleState);
    }

    /**
     * Toggle 'Origin Helper Service' in the 'Start-Up Options' section.
     */
    public void toggleOriginHelperService() {
        clickToggleSlide(START_UP_OPTIONS_SECTION,
                ORIGIN_HELPER_SERVICE_ITEMSTR);
    }

    /**
     * Set 'Origin Helper Service' in the 'Start-Up Options' section to the
     * specified state.
     *
     * @param toggleState Use true to set the toggle state to on. Use false to
     * set the toggle state to off.
     */
    public void setOriginHelperService(boolean toggleState) {
        setToggleSlide(START_UP_OPTIONS_SECTION,
                ORIGIN_HELPER_SERVICE_ITEMSTR,
                toggleState);
    }

    /**
     * Selects a language from the language dropdown.
     *
     * @param lang The language to select
     */
    public void selectLanguage(LanguageEnum lang) {
        // When you select the menu, Origin switches focus to a pop up Qt Dialog
        // which suspends the main window, also suspending the driver making it
        // never return. Instead, we use arrow keys and enter which do return.
        WebElement dropdown = waitForElementClickable(LANGUAGE_DROPDOWN);
        Select select = new Select(dropdown);
        List<String> options = select.getOptions().stream()
                .map(x -> x.getAttribute("value"))
                .collect(Collectors.toList());

        int fromIndex = options.indexOf(select.getFirstSelectedOption().getAttribute("value"));
        int toIndex = options.indexOf(lang.getClientOptionValue());
        int difference = toIndex - fromIndex;
        Keys key = difference > 0 ? Keys.ARROW_DOWN : Keys.ARROW_UP;

        dropdown.click();
        for (int i = 0; i < Math.abs(difference); ++i) {
            dropdown.sendKeys(key);
        }
        dropdown.sendKeys(Keys.ENTER);
        sleep(1000); // Wait for the window to pop up
        // Prepare the state to commit the language change
        OriginClient.getInstance(driver).setStagedLanguage(lang);
    }

    /**
     * Verify all languages in the 'Language' dropdown are localized.
     *
     * @return true if all languages in the dropdown are localized, false otherwise
     */
    public boolean verifyAllLanguagesLocalized() {
        WebElement dropdown = waitForElementClickable(LANGUAGE_DROPDOWN);
        Select select = new Select(dropdown);
        Map<String, String> optionsMap = select.getOptions().stream()
                .collect(Collectors.toMap(x -> x.getAttribute("value").split(":")[1], x -> x.getText()));
        return optionsMap.entrySet().stream()
                .allMatch(x -> StringHelper.containsIgnoreCase(x.getValue(), LanguageInfo.LanguageEnum.parseCode(x.getKey()).getLocalName()));
    }

    /**
     * Returns the current language selected.
     *
     * @return The currently selected {@link LanguageEnum}
     */
    public LanguageEnum getCurrentLanguage() {
        Select dropdown = new Select(waitForElementClickable(LANGUAGE_DROPDOWN));
        return LanguageEnum.valueOf(dropdown
                .getFirstSelectedOption()
                .getText()
                .toUpperCase()
                .replace(' ', '_'));
    }

    /**
     * Verify that 'Automatic Game Updates' in the 'Client Update' section is
     * set to the correct state.
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     * @return true if the toggle state matches the expected state, false otherwise
     */
    public boolean verifyKeepGamesUpToDate(boolean expectedState) {
        return waitIsElementVisible(By.cssSelector(AUTO_UPDATE_ON_CSS_TMPL), 15) == expectedState;
    }

    /**
     * Verify that 'Automatically Update Origin' in the 'Client Update' section
     * is set to the correct state.
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     * @return true if the toggle state matches the expected state, false otherwise
     */
    public boolean verifyOriginGamesUpToDate(boolean expectedState) {
        return verifyToggleSlideState(CLIENT_UPDATE_SECTION,
                KEEP_ORIGIN_UP_TO_DATE_ITEMSTR,
                expectedState);
    }

    /**
     * Verify that 'Participate in Origin Clients Betas' in the 'Client Update'
     * section is set to the correct state.
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     * @return true if the toggle state matches the expected state, false otherwise
     */
    public boolean verifyParticipateInOriginClientBetas(boolean expectedState) {
        return verifyToggleSlideState(CLIENT_UPDATE_SECTION,
                PARTICIPATE_IN_ORIGIN_CLIENT_BETAS_ITEMSTR,
                expectedState);
    }

    /**
     * Verify that 'Automatically Start Origin' in the 'Start-Up Options'
     * section is set to the correct state.
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     * @return true if the toggle state matches the expected state, false otherwise
     */
    public boolean verifyAutomaticallyStartOrigin(boolean expectedState) {
        return verifyToggleSlideState(START_UP_OPTIONS_SECTION,
                AUTOMATICALLY_START_ORIGIN_ITEMSTR,
                expectedState);
    }

    /**
     * Verify that 'Origin Helper Service' in the 'Start-Up Options' section is
     * set to the correct state.
     *
     * @param expectedState The expected state for the toggle. True if the
     * toggle is expected to be on. False if the toggle is expected to be off
     * @return true if the toggle state matches the expected state, false otherwise
     */
    public boolean verifyOriginHelperService(boolean expectedState) {
        return verifyToggleSlideState(START_UP_OPTIONS_SECTION,
                ORIGIN_HELPER_SERVICE_ITEMSTR,
                expectedState);
    }

    /**
     * Toggle all toggle slides in this settings page and verify 'Notifcation
     * Toast' flashes each time.
     *
     * @return true if and only if every toggle results in a 'Notification
     * Toast' flash, false otherwise
     */
    public boolean toggleAppSettingsAndVerifyNotifications() {
        toggleShowOriginAfterGameplay();
        boolean notificationsFlashed = verifyNotificationToastFlashed("Application - Application Settings - Show Origin after gameplay");

        // On low resolution (our test machines), SocialHubMinimized covered slide
        // toggles can no longer be clicked. Solution is to scroll to bottom
        scrollToBottom();
        
        toggleKeepGamesUpToDate();
        notificationsFlashed &= verifyNotificationToastFlashed("Application - Client Update - Automatic game updates");

        toggleKeepOriginUpToDate();
        notificationsFlashed &= verifyNotificationToastFlashed("Application - Client Update - Automatically update Origin");

        toggleParticipateInOriginClientsBetas();
        notificationsFlashed &= verifyNotificationToastFlashed("Application - Client Update - Participate in Origin client betas");

        toggleAutomaticallyStartOrigin();
        notificationsFlashed &= verifyNotificationToastFlashed("Application - Start-Up Options - Automatically start Origin");

        toggleOriginHelperService();
        notificationsFlashed &= verifyNotificationToastFlashed("Application - Start-Up Options - Origin Helper service");

        return notificationsFlashed;

    }

    /**
     * Sets the download speed of the client out of game
     *
     * @param speedValue the element value of the select option. For example
     * "5000000" is 5 MB/s
     */
    public void setDownloadThrottleOutOfGame(String speedValue) {
        WebElement dropdown = waitForElementClickable(DOWNLOAD_RESTRICTION_SELECTION_DROPDOWN);
        Select select = new Select(dropdown);
        int index = IntStream.range(0, select.getOptions().size()).filter(i -> select.getOptions().get(i).getAttribute("value").equals(speedValue)).findFirst().getAsInt();
        select.selectByIndex(index);
    }
}
