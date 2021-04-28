package com.ea.originx.automation.lib.pageobjects.settings;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object representing the 'Settings - Voice' page.
 *
 * @author palui
 */
public class VoiceSettings extends SettingsPage {

    private static final String VCHAT_INDICATOR_TOGGLE_SLIDE_CSS = "#content #\\#activation div.origin-settings-details div.origin-settings-div-side div.otktoggle.origin-settings-toggle";
    private static final By VCHAT_INDICATOR_TOGGLE_SLIDE_LOCATOR = By.cssSelector(VCHAT_INDICATOR_TOGGLE_SLIDE_CSS);

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public VoiceSettings(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify 'Settings - Voice' page is displayed.
     *
     * @return true if 'Settings - Voice' is displayed, false otherwise
     */
    public boolean verifyVoiceSettingsReached() {
        return getActiveSettingsPageType() == PageType.VOICE;
    }

    /**
     * Toggle 'Voice Chat Indicator' in the 'Capture Settings' section.
     */
    public void toggleVoiceChatIndicator() {
        waitForElementClickable(VCHAT_INDICATOR_TOGGLE_SLIDE_LOCATOR).click();
    }

    /**
     * Toggle all toggle slides in this settings page and verify 'Notifcation
     * Toast' flashes each time.
     *
     * @return true if and only if every toggle results in a 'Notification
     * Toast' flash, false otherwise
     */
    public boolean toggleVoiceSettingsAndVerifyNotifications() {
        // Scroll to bottom to get to the Voice Chat Indicator slide toggle in case it is
        // covered by SocialHubMinimized in a low resolution display
        scrollToBottom();
        toggleVoiceChatIndicator();
        return verifyNotificationToastFlashed("Voice - Capture Settings - Voice chat indicator");
    }
}