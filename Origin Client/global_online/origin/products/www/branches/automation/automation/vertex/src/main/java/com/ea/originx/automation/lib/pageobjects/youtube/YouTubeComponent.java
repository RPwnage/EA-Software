package com.ea.originx.automation.lib.pageobjects.youtube;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.NoSuchElementException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

public class YouTubeComponent extends EAXVxSiteTemplate {
    private By rootElement;
    protected By videoIFrameLocator = By.cssSelector("iframe");
    protected By videoElementLocator = By.cssSelector(".html5-video-container video.html5-main-video");
    protected By videoTitleLocator = By.cssSelector(".ytp-title");
    protected By videoControlsLocator = By.cssSelector(".ytp-chrome-bottom");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public YouTubeComponent(WebDriver driver, By locator) {
        super(driver);
        this.rootElement = locator;
    }

    private WebElement getRootElement(){
        return driver.findElement(rootElement);
    }

    /**
     * <p>
     * Returns iFrame containing the actual video element.
     * </p>
     *
     * @return iFrame containing the actual video element.
     */
    private WebElement getVideoIFrame() {
        driver.switchTo().defaultContent();
        return getElementInShadowDom(getRootElement(), videoIFrameLocator);
    }

    private WebElement getVideoElement() {
        WebElement vidIFrame = getVideoIFrame();
        driver.switchTo().frame(vidIFrame);
        return driver.findElement(videoElementLocator);
    }

    /**
     * <pre>
     * Returns true if and only if the video element exists, is displayed and
     * has non zero size.
     * </pre>
     *
     * @return Whether the vid is displayed
     */
    public boolean isVideoDisplayed() {
        driver.switchTo().frame(getVideoIFrame());
        return isElementVisible(videoElementLocator);
    }

    /**
     * <p>
     * Returns true if title is displayed
     * </p>
     *
     * @return true if title is displayed
     */
    public boolean isTitleDisplayed() {
        driver.switchTo().frame(getVideoIFrame());
        return isElementVisible(videoTitleLocator);
    }

    /**
     * <p>
     * Returns true if video controls are displayed
     * </p>
     *
     * @return true if video controls are displayed
     */
    public boolean isVideoControlsDisplayed() {
        driver.switchTo().frame(getVideoIFrame());
        return isElementVisible(videoControlsLocator);
    }

    /**
     * <p>
     * Get the current time of the playing video
     * </p>
     *
     * @return current time of the playing video
     */
    public int getVideoCurrentTime() {
        WebElement vidElement = getVideoElement();
        double currentTime = (double) ((Double) jsExec.executeScript("return arguments[0].currentTime", vidElement));
        driver.switchTo().defaultContent();
        return (int) currentTime;
    }

    /**
     * <p>
     * Returns whether or not video is actually playing.
     * </p>
     *
     * @return Whether the vid is playing.
     * @throws InterruptedException InterruptedException
     */
    public boolean verifyVideoPlaying() throws InterruptedException {
        //Wait for the video start
        Thread.sleep(3000);
        WebElement vidElement = getVideoElement();
        Long playbackRate = (Long) jsExec.executeScript("return arguments[0].playbackRate", vidElement);
        Boolean isPaused = (Boolean) jsExec.executeScript("return arguments[0].paused", vidElement);

        boolean isPlaying = !isPaused && playbackRate > 0;
        driver.switchTo().defaultContent();
        return isPlaying;
    }

    /**
     * <p>
     * Forces with javascript the video to go directly to the time in parameter
     * </p>
     *
     * @param videoTime videoTime in long
     */
    public void goToVideoTime(Long videoTime) {
        WebElement vidElement = getVideoElement();
        jsExec.executeScript("return arguments[0].currentTime = arguments[1]", vidElement, videoTime);
        driver.switchTo().defaultContent();
    }

    /**
     * <p>
     * Get the duration of the playing video
     * </p>
     *
     * @return duration of the playing video
     */
    public Double getVideoDuration() {
        WebElement vidElement = getVideoElement();
        Double duration = (Double) jsExec.executeScript("return arguments[0].duration", vidElement);
        driver.switchTo().defaultContent();
        return duration;
    }

    /**
     * <p>
     * Clicks to play the video
     * </p>
     */
    public void playVideo() {
        getRootElement().click();
    }
}
