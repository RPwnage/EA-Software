package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;


/**
 * Third Party Payment Provider Redirect Message Dialog
 *
 * @author Seanli
 */
public class CheckoutRedirectMessageDefaultDialog extends Dialog {

    private final static By DIALOG_CONTENT_LOCATOR = By.cssSelector(".origin-dialog-content");
    private final static By TITLE_LOCATOR = By.cssSelector(".origin-dialog-content[redirect-message-type='default'] .otkmodal-header .otkmodal-title");
    private final static String REDIRECT_MESSAGE_TYPE_ATTR = "redirect-message-type";
    private final static String REDIRECT_MESSAGE_TYPE = "default";

    /**
     * Constructor for default type dialog instance.
     *
     * @param driver WebDriver instance
     */
    public CheckoutRedirectMessageDefaultDialog(WebDriver driver){
       super(driver, null, TITLE_LOCATOR);
    }

    /**
     * Verify the type of the redirect message
     *
     * @return True if the type match with the passed in type
     * otherwise
     */
    public boolean verifyRedirectMessageType() {
        return waitForElementClickable(DIALOG_CONTENT_LOCATOR).getAttribute(REDIRECT_MESSAGE_TYPE_ATTR).equals(REDIRECT_MESSAGE_TYPE);
    }

}
