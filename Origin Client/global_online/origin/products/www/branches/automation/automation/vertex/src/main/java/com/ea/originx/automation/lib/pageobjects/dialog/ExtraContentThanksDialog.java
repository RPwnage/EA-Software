package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * A page object that represents the 'Thank You' page after placing successful
 * purchase of a extra content for base game.
 *
 * @author rchoi
 */
public class ExtraContentThanksDialog extends Dialog {

    protected static final By DOWNLOAD_WITH_ORIGIN_BUTTON_LOCATOR = By.cssSelector(".origin-checkoutconfirmation-item-details .otkbtn.otkbtn-primary");
    protected static final String TITLE_STRING = "thank";

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public ExtraContentThanksDialog(WebDriver driver) {
        super(driver);
    }
}