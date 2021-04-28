package com.ea.originx.automation.lib.pageobjects.discover;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.util.List;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Complete Profile' section
 *
 * @author mkalaivanan
 */
public class CompleteProfile extends EAXVxSiteTemplate {

    protected static final String COMPLETE_PROFILE_CSS = "origin-home-get-started-bucket";
    protected static final String GET_STARTED_TASK_CSS = "origin-home-get-started-bucket .origin-getstarted-tile";

    protected static final By COMPLETE_YOUR_PROFILE_LOCATOR = By.cssSelector(COMPLETE_PROFILE_CSS);
    protected static final By GET_STARTED_TASK_ROOT_LOCATOR = By.cssSelector(GET_STARTED_TASK_CSS);
    protected static final By GET_STARTED_TASK_IMAGE_LOCATOR = By.cssSelector("img");
    protected static final By GET_STARTED_TASK_TITLE_LOCATOR = By.cssSelector(".otktitle-3.otktitle-3-strong.origin-tile-support-text-title");
    protected static final By GET_STARTED_TASK_DESCRIPTION_LOCATOR = By.cssSelector(".origin-tile-support-text-paragraph");
    protected static final By GET_STARTED_TASK_CTA_BUTTON_LOCATOR = By.cssSelector(".origin-getstarted-tile-cta-area .otkbtn.otkbtn-primary.getstarted-btn-primary");
    protected static final By GET_STARTED_TASK_REMIND_ME_LATER_LINK_LOCATOR = By.cssSelector(".origin-getstarted-tile-cta-area .otka.getstarted-a.secondary-action");
    protected static final By VIEW_MORE_BUTTON_LOCATOR = By.cssSelector(COMPLETE_PROFILE_CSS + " .l-origin-home-section-more > a");
    protected static final By ACHIEVEMENT_VISIBILITY_TASK_IMAGE_LOCATOR = By.cssSelector(GET_STARTED_TASK_CSS + " img[src*='completeProfile_achievementVisibility.png']");
    protected static final By VERIFY_EMAIL_TASK_IMAGE_LOCATOR = By.cssSelector(GET_STARTED_TASK_CSS + " img[src*='completeProfile_emailVerification.png']");
    protected static final By PROFILE_VISIBILITY_TASK_IMAGE_LOCATOR = By.cssSelector(GET_STARTED_TASK_CSS + " img[src*='completeProfile_profileVisibility.png']");
    protected static final By GET_STARTED_TASK_CTA_CONFIRMATION_INDICATOR_LOCATOR = By.cssSelector(".origin-getstarted-tile-cta-confirmation .otkicon-email");
    protected static final By ROOT_PARENT_TASK_CSS_LOCATOR = By.xpath("..");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CompleteProfile(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify all visible task tiles have images.
     *
     * @return true if all visible tiles have images, false otherwise
     */
    public boolean verifyTaskImageExistsForAllVisibleTasks() {

        return isElementVisibleOnAllTaskTiles(GET_STARTED_TASK_IMAGE_LOCATOR);
    }

    /**
     * Verify all visible task tiles have description.
     *
     * @return true if all visible tiles have description, false otherwise
     */
    public boolean verifyTaskDescriptionExistsForAllVisibleTasks() {
        return isElementVisibleOnAllTaskTiles(GET_STARTED_TASK_DESCRIPTION_LOCATOR);
    }

    /**
     * Get all the visible complete profile tasks.
     *
     * @return list of web elements representing all tiles, false otherwise
     */
    private List<WebElement> getCompleteSectionProfileTasks() {
        return waitForAllElementsVisible(GET_STARTED_TASK_ROOT_LOCATOR);
    }

    /**
     * Verify all visible task tiles have title/name.
     *
     * @return true if all visible task tiles have title, false otherwise
     */
    public boolean verifyTaskNameExistsForAllVisibleTasks() {
        return isElementVisibleOnAllTaskTiles(GET_STARTED_TASK_TITLE_LOCATOR);
    }

    /**
     * Verify all visible tiles have CTA buttons when hovered over.
     *
     * @return true if all tiles have CTA buttons, false otherwise
     */
    public boolean verifyTaskCtaButtonForAllVisibleTasks() {
        List<WebElement> completeProfileTiles = getCompleteSectionProfileTasks();
        for (WebElement completeProfileTile : completeProfileTiles) {
            hoverOnElement(completeProfileTile);
            if (!waitIsElementVisible(completeProfileTile.findElement(GET_STARTED_TASK_CTA_BUTTON_LOCATOR))) {
                return false;
            }
        }
        return true;
    }

    /**
     * Determine if an element is present on all task tiles.
     *
     * @param cssSelector The CSS selector of the element to check
     * @return true if expected element is present on all visible task tiles,
     * false otherwise
     */
    private boolean isElementVisibleOnAllTaskTiles(By cssSelector) {
        List<WebElement> completeProfileTiles = getCompleteSectionProfileTasks();
        for (WebElement completeProfileTile : completeProfileTiles) {
            if (!waitIsElementVisible(completeProfileTile.findElement(cssSelector))) {
                return false;
            }
        }
        return true;

    }

    /**
     * Click on the 'View More' button.
     */
    public void clickViewMoreButton() {
        waitForElementClickable(VIEW_MORE_BUTTON_LOCATOR).click();
    }

    /**
     * Get the number of visible task tiles.
     *
     * @return Number of task tiles visible
     */
    public int getVisibleTasksCount() {
        return getCompleteSectionProfileTasks().size();
    }

    /**
     * Click on the CTA button on the 'Verify Email Address' task.
     */
    public void clickCtaButtonVerifyEmailAddressTask() {
        clickCtaButtonOnGetStartedTask(VERIFY_EMAIL_TASK_IMAGE_LOCATOR);
    }

    /**
     * Verify CTA Completion indicator is visible after clicking on CTA button
     * on the 'Verify Email Address' task.
     *
     * @return true if indicator is visible, false otherwise
     */
    public boolean verifyCtaCompletionIndicatorOnVerifyEmailAddressTask() {
        return waitIsElementVisible(waitForElementVisible(VERIFY_EMAIL_TASK_IMAGE_LOCATOR)
                .findElement(ROOT_PARENT_TASK_CSS_LOCATOR)
                .findElement(GET_STARTED_TASK_CTA_CONFIRMATION_INDICATOR_LOCATOR));
    }

    /**
     * Verify if task image tile is visible.
     *
     * @param cssSelectorForTaskTileImage The CSS selector for the task image tile
     * @return true if visible, false otherwise
     */
    private boolean isTaskVisible(By cssSelectorForTaskTileImage) {
        return isElementVisible(cssSelectorForTaskTileImage);
    }

    /**
     * Verify if verify your email address task image is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyEmailAddressTaskVisible() {
        return isTaskVisible(VERIFY_EMAIL_TASK_IMAGE_LOCATOR);
    }

    /**
     * Verify if 'Profile Visibility' task image is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean isProfileVisibilityTaskVisible() {
        return isTaskVisible(PROFILE_VISIBILITY_TASK_IMAGE_LOCATOR);
    }

    /**
     * Click on the CTA button on 'Achievement Visibility' task.
     */
    public void clickCtaButtonOnAchievementVisiblityTask() {
        clickCtaButtonOnGetStartedTask(ACHIEVEMENT_VISIBILITY_TASK_IMAGE_LOCATOR);
    }

    /**
     * Click the CTA button on a task tile by hovering over it
     *
     * @param cssSelector The CSS selector of the tile
     */
    private void clickCtaButtonOnGetStartedTask(By cssSelector) {
        WebElement signUpForEmailTile = waitForElementVisible(cssSelector);
        hoverOnElement(signUpForEmailTile);
        clickOnElement(signUpForEmailTile.findElement(ROOT_PARENT_TASK_CSS_LOCATOR).findElement(GET_STARTED_TASK_CTA_BUTTON_LOCATOR));
    }

    /**
     * Scroll to the 'Complete Your Profile' section.
     */
    public void scrollToCompleteYourAccountSection() {
        scrollToElement(waitForElementVisible(COMPLETE_YOUR_PROFILE_LOCATOR));
    }

    /**
     * Verify the 'Remind Me Later' button is available on the 'Profile Visibility' task.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyRemindMeLaterButtonOnProfileVisibilityTask() {
        hoverOnElement(getProfileVisibilityTask());
        return waitIsElementVisible(getProfileVisibilityTask()
                .findElement(ROOT_PARENT_TASK_CSS_LOCATOR)
                .findElement(GET_STARTED_TASK_REMIND_ME_LATER_LINK_LOCATOR)
        );
    }

    /**
     * Verify the CTA button is visible on the 'Profile Visibility' task.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyCtaButtonOnProfileVisibilityTask() {
        hoverOnElement(getProfileVisibilityTask());
        return waitIsElementVisible(getProfileVisibilityTask()
                .findElement(ROOT_PARENT_TASK_CSS_LOCATOR)
                .findElement(GET_STARTED_TASK_CTA_BUTTON_LOCATOR)
        );
    }

    /**
     * Get the 'Profile Visibility' task element.
     *
     * @return The 'Profile Visibility' task web element
     */
    private WebElement getProfileVisibilityTask() {
        return waitForElementVisible(PROFILE_VISIBILITY_TASK_IMAGE_LOCATOR);
    }

    /**
     * Verify the 'Profile Visibility' task is visible.
     *
     * @return true if visible, false otherwise
     */
    public boolean verifyProfileVisibilityTaskVisible() {
        return waitIsElementVisible(PROFILE_VISIBILITY_TASK_IMAGE_LOCATOR);
    }

    /**
     * Click on the CTA button by hovering over the 'Profile Visibility' task.
     */
    public void clickCtaButtonOnProfileVisibility() {
        clickCtaButtonOnGetStartedTask(PROFILE_VISIBILITY_TASK_IMAGE_LOCATOR);
    }
}