package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.macroaction.MacroAgeGate;
import org.openqa.selenium.By;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.WebDriver;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.WebElement;
import java.text.ParseException;

/**
 * Represents the dialog that appears when you click on/maximize a media item in
 * the PDP carousel.
 *
 * @author jmittertreiner
 */
public class MediaDialog extends Dialog {

    private static final By LOCATOR = By.id("dialog");
    private static final By DIALOG_CONTENT = By.cssSelector(".otkmodal-content");
    private static String AGE_GATE = ".origin-dialog-content-media-carousel .origin-dialog-content-media-carousel-item-active .origin-agegate";
    private static final By AGE_GATE_LOCATOR = By.cssSelector(AGE_GATE);
    private static final By AGE_MESSAGE_LOCATOR = By.cssSelector(AGE_GATE + ".origin-agegate-underage-yes");
    private final By VIDEO_AGEGATE_SELECT_LOCATOR = By.cssSelector(AGE_GATE + " .otkselect.origin-agegate-select");
    private final By VIDEO_AGEGATE_OKAY_BUTTON_LOCATOR = By.cssSelector(AGE_GATE + " .origin-agegate-buttons");
    private final By VIDEO_ID_LOCATOR = By.cssSelector(".origin-dialog-content-media-carousel.media-type-video ul li.origin-dialog-content-media-carousel-item-active div");
    private final By VIDEO_PLAYER_IFRAME_LOCATOR = By.cssSelector(".origin-dialog-content-media-carousel.media-type-video ul li.origin-dialog-content-media-carousel-item-active .origin-youtube-player");
    private static final By MEDIA_MODAL_LOCATOR = By.cssSelector("li.origin-dialog-content-media-carousel-item.origin-dialog-content-media-carousel-item-active");

    // Video Buttons
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector("origin-dialog .otkmodal-close .otkicon.otkicon-close");
    private final By NEXT_ARROW_BUTTON_LOCATOR = By.cssSelector(".origin-dialog-content-media-carousel-next");
    private final By PREV_ARROW_BUTTON_LOCATOR = By.cssSelector(".origin-dialog-content-media-carousel-prev");

    public MediaDialog(WebDriver driver) {
        super(driver, LOCATOR, null, CLOSE_BUTTON_LOCATOR);
    }

    /**
     * Returns if the age requirement has been met.
     *
     * @return false if the dialog is displaying that the user does not meet the
     * needed age requirements for the media, else true
     */
    public boolean isAgeRequirementsMet() {
        if (!waitIsVisible()) {
            return false;
        }

        return !waitIsElementVisible(AGE_MESSAGE_LOCATOR);
    }

    /**
     * Checks to see if the age gate is visible.
     *
     * @return true if age gate is presented to user, false otherwise
     */
    public boolean verifyAgeGate() {
        if (!waitIsVisible()) {
            return false;
        }

        return waitIsElementVisible(AGE_GATE_LOCATOR);
    }

    /**
     * Enters the given birthday into the video age gate.
     *
     * @param birthday The birthday to enter
     * @throws java.text.ParseException
     */
    public void enterVideoAgeGateBirthday(String birthday) throws ParseException {
        MacroAgeGate.enterVideoAgeGateBirthday(birthday, waitForAllElementsVisible(VIDEO_AGEGATE_SELECT_LOCATOR), waitForElementClickable(VIDEO_AGEGATE_OKAY_BUTTON_LOCATOR));
    }

    /**
     * To get the video-id(source) of the video being loaded in the dialog
     *
     * @return the video-id of the video-loaded
     */
    public String getVideoID() {
        return waitForElementVisible(VIDEO_ID_LOCATOR).getAttribute("video-id");
    }

    /**
     * Click the 'Next' arrow to scroll to the next media carousel item.
     */
    public void clickNextArrow() {
        waitForElementClickable(NEXT_ARROW_BUTTON_LOCATOR).click();
    }

    /**
     * Click the prev arrow to scroll to the previous media carousel item
     */
    public void clickPrevArrow() {
        waitForElementClickable(PREV_ARROW_BUTTON_LOCATOR).click();
    }

    /**
     * Tests to see if the dialog content is in a full screen mode
     *
     * @return true if dialog content size is same as the view port size
     */
    public boolean isContentInFullScreenMode(Dimension viewPortSize) {
        Dimension contentSize = waitForElementVisible(DIALOG_CONTENT).getSize();
        return viewPortSize.equals(contentSize);
    }

    /**
     * Tests to see if the age gate is shown in a full screen mode
     *
     * @return true if age gate size is same as content size
     */
    public boolean ageGateIsInFullScreenMode() {
        Dimension contentSize = waitForElementVisible(DIALOG_CONTENT).getSize();
        Dimension ageGateSize = waitForElementVisible(AGE_GATE_LOCATOR).getSize();
        return contentSize.equals(ageGateSize);
    }

    /**
     * Play the video
     */
    public void playVideo() {
        waitForElementClickable(VIDEO_ID_LOCATOR).click();
    }

    /**
     * Test to see if the video is in a pause mode
     *
     * @return true if the video is in a pause mode
     */
    public boolean verifyVideoIsPaused() {
        WebElement videoiFrame = waitForElementVisible(VIDEO_PLAYER_IFRAME_LOCATOR);
        WebDriver iFrameDriver = this.driver.switchTo().frame(videoiFrame);
        String playButtonLabel = iFrameDriver.findElement(By.cssSelector(".ytp-play-button ")).getAttribute("aria-label");
        return playButtonLabel.equals("Play");
    }

    /**
     * Test to see if the video is in a playing mode
     *
     * @return true if the video is in a playing mode
     */
    public boolean verifyVideoIsPlaying(){
        WebElement videoiFrame = waitForElementVisible(VIDEO_PLAYER_IFRAME_LOCATOR);
        WebDriver iFrameDriver = this.driver.switchTo().frame(videoiFrame);
        String videoPlayerLabel = iFrameDriver.findElement(By.cssSelector(".html5-video-player")).getAttribute("class");
        return videoPlayerLabel.contains("playing-mode");
    }
    
    /**
     * Verify if the video is allowing a full-window mode
     *
     * @return true if the video attribute 'allowfullscreen' equals '1', and
     * 'width' and 'height' attributes values are set to '100%', false otherwise
     */
    public boolean verifyVideoAllowingFullScreen() {
        WebElement videoIFrame = waitForElementVisible(VIDEO_PLAYER_IFRAME_LOCATOR);
        String widthValue = videoIFrame.getAttribute("width");
        String heightValue = videoIFrame.getAttribute("height");
        String allowFullScreenValue = videoIFrame.getAttribute("allowfullscreen");
        return widthValue.equals("100%") && heightValue.equals("100%") && allowFullScreenValue.equals("1");
    }

    /**
     * Verifies that the Media Modal is visible
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyMediaDialogVisible(){
        return waitIsElementVisible(MEDIA_MODAL_LOCATOR);
    }
}
