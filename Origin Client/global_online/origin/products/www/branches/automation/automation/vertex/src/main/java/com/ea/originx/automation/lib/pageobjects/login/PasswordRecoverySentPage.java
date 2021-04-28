package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the page object containing the successful message text after
 * entering email and captcha to recover password.
 *
 * @author mkalaivanan
 */
public class PasswordRecoverySentPage extends EAXVxSiteTemplate {

    protected static final By PASSWORD_RECOVERY_SENT_MESSAGE_LOCATOR = By.cssSelector("#panel-reset-password-requestSent #passwordRecoverSent .otktitle.otktitle-2");
    protected static final By BACK_BUTTON_LOCATOR = By.cssSelector("#backToLoginBtn");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PasswordRecoverySentPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for page to load.
     */
    public void waitForPage() {
        Waits.waitForWindowThatHasElement(driver, PASSWORD_RECOVERY_SENT_MESSAGE_LOCATOR);
    }

    /**
     * Verify 'Password Recovery Sent' message is visible.
     *
     * @return true if the 'Password Recovery Sent' message is visible,
     * false otherwise
     */
    public boolean verifyPasswordRecoverySentMessage() {
        return waitForElementVisible(PASSWORD_RECOVERY_SENT_MESSAGE_LOCATOR).getText().equalsIgnoreCase("Password Recovery Sent");
    }

    /**
     * Click on the 'Back' button.
     */
    public void clickBackButton() {
        waitForElementClickable(BACK_BUTTON_LOCATOR).click();
    }
}
