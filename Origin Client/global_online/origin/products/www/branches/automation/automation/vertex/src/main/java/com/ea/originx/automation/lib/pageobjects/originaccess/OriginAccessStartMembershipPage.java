package com.ea.originx.automation.lib.pageobjects.originaccess;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object which represents the 'Origin Access Start Membership' page.
 * 
 * @author mdobre
 */
public class OriginAccessStartMembershipPage extends EAXVxSiteTemplate{
    
    protected static final By IFRAME_LOCATOR = By.cssSelector("origin-iframe-modal .origin-iframemodal .origin-iframemodal-content iframe");
    protected static final By IFRAME_TITLE_LOCATOR = By.cssSelector(".checkout-content .otkmodal-header .otkmodal-title");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-dialog.otkmodal-lg .otkicon.otkicon-closecircle");
    protected static final By START_MEMBERSHIP_BUTTON_LOCATOR = By.cssSelector(".checkout-content.otkmodal-content .otkmodal-footer .otkbtn.otkbtn-primary");
    protected static final String START_MEMBERSHIP_BUTTON_TEXT = "Start Membership";

    public OriginAccessStartMembershipPage(WebDriver driver) {
        super(driver);
    }
    
    /**
     * Wait for page to load.
     */
    public void waitOriginAccessStartMembershipPagePageToLoad() {
        waitForPageToLoad();
        // if current frame is iframe, it does not do anything
        if (!checkIfFrameIsActive("iframe")) {
            waitForFrameAndSwitchToIt(waitForElementVisible(IFRAME_LOCATOR, 10));
        }
    }
    
    /**
     * Verify user reached the 'Origin Access Start Membership' page.
     * 
     * @return true if the page reached is the 'Origin Access Start Membership', false otherwise
     */
    public boolean verifyOriginAccessStartMembershipPageReached() {
        return waitForElementVisible(START_MEMBERSHIP_BUTTON_LOCATOR).getText().equals(START_MEMBERSHIP_BUTTON_TEXT);
    }
    
    /**
     * Click on the 'Start Membership' button.
     */
    public void clickOnStartMembershipButton() {
        waitForElementClickable(START_MEMBERSHIP_BUTTON_LOCATOR).click();
    }
    
    /**
     * Click on the 'X' button in order to close the page.
     */
    public void clickCloseButton() {
        driver.switchTo().defaultContent();
        waitForElementVisible(CLOSE_BUTTON_LOCATOR).click();
    }    
}