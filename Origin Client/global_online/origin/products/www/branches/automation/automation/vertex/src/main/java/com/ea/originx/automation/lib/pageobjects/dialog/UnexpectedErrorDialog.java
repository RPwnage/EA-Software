package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Dialog indicating that an unexpected error has occurred.
 *
 * @author glivingstone
 */
public class UnexpectedErrorDialog extends Dialog {

    protected static final By PROBLEM_ENCOUNTERED_LOCATOR = By.cssSelector("origin-dialog-content-prompt[header*='encountered a problem']");
    protected static  By WARNING_TITLE_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-title");
    protected static final By BODY_TEXT_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body");
    protected static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-footer .otkmodal-btn-no");

    public UnexpectedErrorDialog(WebDriver driver) {
        super(driver, PROBLEM_ENCOUNTERED_LOCATOR, null, CLOSE_BUTTON_LOCATOR);
    }
}