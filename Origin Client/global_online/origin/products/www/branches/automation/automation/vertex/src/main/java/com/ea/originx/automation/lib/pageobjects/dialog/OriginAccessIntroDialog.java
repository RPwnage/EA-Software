package com.ea.originx.automation.lib.pageobjects.dialog;

import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;

/**
 * Represents the 'Origin Access Intro' dialog, appearing after finishing purchasing Origin
 * Access.
 *
 * @author glivingstone
 */
public class OriginAccessIntroDialog extends Dialog {

    protected static final By ORIGIN_ACCESS_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-intro");
    protected static final By LETS_GO_BUTTON_LOCATOR = By.cssSelector(".otkmodal-content .origin-store-access-nux-intro .otkbtn-primary");

    public OriginAccessIntroDialog(WebDriver driver) {
        super(driver, ORIGIN_ACCESS_LOCATOR);
    }

    /**
     * Click the 'Let's Go' button.
     */
    public void clickLetsGoButton(){
        waitForElementClickable(LETS_GO_BUTTON_LOCATOR).click();
    }
}