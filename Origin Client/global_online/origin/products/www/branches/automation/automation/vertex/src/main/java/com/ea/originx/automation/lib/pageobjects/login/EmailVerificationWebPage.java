package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the web page that opens when clicking the 'Verify Parent Email'
 * link on a email sent to the parent of a newly registered underage user.
 *
 * @author palui
 */
public class EmailVerificationWebPage extends EAXVxSiteTemplate {

    protected static final String CONTENT_CSS = ".origin-com #panel-profile-confirmation .panel-content";
    protected static final By TITLE_LOCATOR = By.cssSelector(CONTENT_CSS + " h1");
    protected static final By RESULT_TEXT_LOCATOR = By.cssSelector(" #resultText");

    protected static final String EMAIL_VERIFICATION_WEB_PAGE_URL_REGEX = "^https://signin.*ea.com/p/web/emailVerification.*";
    protected static final String EXPECTED_TITLE = "Email Verification";
    protected static final String[] EXPECTED_SUCCESS_TEXT_KEYWORDS = {"Success", "verified"};

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public EmailVerificationWebPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Wait for the 'Email Verification' web page to load.
     */
    public void waitForEmailVerificationWebPageToLoad() {
        waitForPageThatMatches(EMAIL_VERIFICATION_WEB_PAGE_URL_REGEX, 60);
    }

    /**
     * Verify 'Email Verification' web page title matches the expected title.
     *
     * @return true if title matches, false otherwise
     */
    public boolean verifyTitle() {
        return waitForElementVisible(TITLE_LOCATOR).getText().equalsIgnoreCase(EXPECTED_TITLE);
    }

    /**
     * Verify 'Email Verification' result text indicates success.
     *
     * @return true if successful, false otherwise
     */
    public boolean verifySuccessfulResult() {
        return StringHelper.containsIgnoreCase(waitForElementVisible(RESULT_TEXT_LOCATOR).getText(),
                EXPECTED_SUCCESS_TEXT_KEYWORDS);
    }
}