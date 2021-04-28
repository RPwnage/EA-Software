package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import java.lang.invoke.MethodHandles;

/**
 * Represents the page that is displayed after a user clicks the prompt to open
 * a gift from the 'My Home' page.
 *
 * @author jmitterteriner
 */
public class OpenGiftPage extends EAXVxSiteTemplate {

    private static final By OPEN_GIFT_PAGE_LOCATOR = By.cssSelector(".l-origin-unveiling");
    private static final By PACK_ART_LOCATOR = By.cssSelector("img.origin-unveiling-posterart-image");
    private static final String AVATAR_TMPL = "img.otkavatar-img";
    private static final By SENDER_GAME_INFORMATION_LOCATOR = By.cssSelector(".origin-unveiling-sendergameinfo");
    private static final By CUSTOM_MESSAGE_LOCATOR = By.cssSelector(".origin-unveiling-senderquote");
    private static final By DOWNLOAD_NOW_LOCATOR = By.cssSelector(".l-origin-unveiling .origin-cta-primary .otkbtn-primary");
    private static final By DOWNLOAD_LATER_LOCATOR = By.cssSelector(".origin-unveiling-downloadlaterlink");
    private static final By ADDED_TO_LIBRARY_MESSAGE_LOCATOR = By.cssSelector(".origin-unveiling-gameaddedmessage");
    private static final By CLOSE_CIRCLE_BUTTON_LOCATOR = By.cssSelector("div.otkmodal-close.origin-unveiling-closeicon");

    // Logging
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    public OpenGiftPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the 'Open Gift' page to be visible.
     *
     * @return true if 'Open Gift' page is visible, false otherwise
     */
    public boolean waitForVisible() {
        waitForPageToLoad();
        return waitIsElementVisible(OPEN_GIFT_PAGE_LOCATOR);
    }

    /**
     * Verify the pack art is displayed.
     *
     * @return true if the pack art is displayed, false otherwise
     */
    public boolean verifyPackArtDisplayed() {
        return waitIsElementVisible(PACK_ART_LOCATOR);
    }

    /**
     * Verify the pack art is displayed given an image.
     *
     * @param image Image to compare pack art against
     * @return true if the correct pack art is displayed, false otherwise
     */
    public boolean verifyPackArtDisplayed(String image) {
        WebElement packArt = waitForElementVisible(PACK_ART_LOCATOR);

        if (packArt != null) {
            return packArt.getAttribute("src").equals(image);
        } else {
            _log.debug("Could not find pack art on gifting page");
            return false;
        }
    }

    /**
     * Verify the avatar matches the given username.
     *
     * @param username The username we expect the avatar to match
     * @return true if the avatar image 'alt' tag matches the given username,
     * false otherwise
     */
    public boolean verifyAvatarMatchesUser(String username) {
        WebElement senderAvatar = waitForElementVisible(By.cssSelector(AVATAR_TMPL));
        String altText = senderAvatar.getAttribute("alt");
        if (altText == null || altText.isEmpty()) {
            _log.error("Could not find name attribute for Avatar");
            return false;
        }
        return StringUtils.containsIgnoreCase(username, altText);
    }

    /**
     * Verify the information contains the given text.
     *
     * @param expected The String we expect in the information string
     * @return true if the information text contains the given string,
     * false otherwise
     */
    public boolean verifyInformationContains(String expected) {
        String text = waitForElementVisible(SENDER_GAME_INFORMATION_LOCATOR).getText();
        return StringHelper.containsIgnoreCase(text, expected);
    }

    /**
     * Verify the gift message matches the given message.
     *
     * @param message The expected message
     * @return true if the messages match, false otherwise
     */
    public boolean verifyMessage(String message) {
        return waitForElementVisible(CUSTOM_MESSAGE_LOCATOR).getText().contains(message);
    }

    /**
     * Click 'Download Now'.
     */
    public void clickDownloadNow() {
        waitForElementClickable(DOWNLOAD_NOW_LOCATOR).click();
    }

    /**
     * Click 'Download Later' button which adds the game to the 'Game Library'
     * but doesn't download the game.
     */
    public void clickDownloadLater() {
        clickButtonWithRetries(DOWNLOAD_LATER_LOCATOR, OPEN_GIFT_PAGE_LOCATOR);
    }

    /**
     * Verify that the close button is visible
     *
     * @return true if the close button is visible, false otherwise
     */
    public boolean verifyCloseButtonVisible(){
        return waitIsElementVisible(CLOSE_CIRCLE_BUTTON_LOCATOR);
    }

    /**
     * Click the close button at the top right of the page to exit the unveiling flow
     */
    public void clickCloseButton(){
        waitForElementClickable(CLOSE_CIRCLE_BUTTON_LOCATOR).click();
    }

    /**
     * Verify messaging that the game has been added to library is visible.
     *
     * @return true if the message is visible, false otherwise
     */
    public boolean verifyAddedToLibraryMessageVisible(){
        return waitIsElementVisible(ADDED_TO_LIBRARY_MESSAGE_LOCATOR);
    }

    /**
     * Check to see if the unveil page takes up the full window by comparing
     * top, left, right, bottom properties against 0px.
     *
     * @return true if unveil size takes up window size, false otherwise
     */
    public boolean verifyFullScreenUnveil() {
        WebElement unveilPage = waitForElementVisible(OPEN_GIFT_PAGE_LOCATOR);
        _log.debug("top: " + unveilPage.getCssValue("top"));
        _log.debug("left: " + unveilPage.getCssValue("left"));
        _log.debug("right: " + unveilPage.getCssValue("right"));
        _log.debug("bottom: " + unveilPage.getCssValue("bottom"));
        return "0px".equals(unveilPage.getCssValue("top")) && 
                "0px".equals(unveilPage.getCssValue("left"))&& 
                "0px".equals(unveilPage.getCssValue("right"))&& 
                "0px".equals(unveilPage.getCssValue("bottom"));
    }

    /**
     * Verify the 'Download Now'/'Install Origin & Play' button is present.
     *
     * @return true if the 'Download Now'/'Install Origin & Play' button is present, false otherwise
     */
    public boolean verifyPrimaryCTA() {
        return waitIsElementPresent(DOWNLOAD_NOW_LOCATOR);
    }

    /**
     * Check to see if the the 'Add to Library' and 'Download Later' link appears.
     *
     * @return true if link appears, false otherwise
     */
    public boolean verifyAddDownloadLater() {
        return waitIsElementVisible(DOWNLOAD_LATER_LOCATOR);
    }
}