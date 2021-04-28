package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represents the 'Base Game Message' dialog, which pops up when trying to
 * purchase a DLC without having already purchased the base game. Can also
 * use this dialog as the dialog that pops up when purchasing extra content.
 *
 * @author palui
 */
public final class BaseGameMessageDialog extends Dialog {

    protected static final String DIALOG_CONTENT_CSS = ".otkmodal-content";
    protected static final String CONTENT_TITLE_CSS = DIALOG_CONTENT_CSS + " h4.otkmodal-title";
    protected static final By DIALOG_CONTENT_LOCATOR = By.cssSelector(DIALOG_CONTENT_CSS);
    protected static final By CONTENT_TITLE_LOCATOR = By.cssSelector(CONTENT_TITLE_CSS);
    protected static final By BASE_PDP_LINK_LOCATOR = By.cssSelector(CONTENT_TITLE_CSS + " > a");
    protected static final By DESCRIPTION_LOCATOR = By.cssSelector(DIALOG_CONTENT_CSS + " > h2.origin-dialog-content-pp-text");
    protected static final By CONTINUE_PURCHASE_CTA_LOCATOR = By.cssSelector(DIALOG_CONTENT_CSS + " div.otkmodal-footer .otkbtn-primary");

    protected static final String EXPECTED_TITLE_PATTERN_TMPL = ".*need %s to use.*%s"; // need bast to play dlc
    protected static final String EXPECTED_DESCRIPTION_PATTERN_TMPL = ".*can still buy %s.*until you get.*%s"; // play dlc need to get base

    protected String dlcName;
    protected String baseName;

    /**
     * Constructor
     *
     * @param driver Selenium Driver
     * @param dlcName Name of DLC game that appears in title and description
     * @param baseName Name of base game that appears in title and description
     */
    public BaseGameMessageDialog(WebDriver driver, String dlcName, String baseName) {
        super(driver, DIALOG_CONTENT_LOCATOR, CONTENT_TITLE_LOCATOR);
        this.dlcName = dlcName;
        this.baseName = baseName;
    }

    /**
     * Verify dialog title matches the expected 'Base Game Message' dialog
     * title. The title will contain both the dlc game name and the base game
     * name.
     *
     * @return true if title matches, false otherwise
     */
    public boolean verifyTitle() {
        String pattern = String.format(EXPECTED_TITLE_PATTERN_TMPL, baseName, dlcName);
        return StringHelper.matchesIgnoreCase(getTitle(), pattern);
    }

    /**
     * Verify dialog description matches the expected 'Base Game Message' dialog
     * description. The description will contain both the base game name and the
     * DLC game name
     *
     * @return true if description matches, false otherwise
     */
    public boolean verifyDescription() {
        String description = waitForElementVisible(DESCRIPTION_LOCATOR).getText();
        String pattern = String.format(EXPECTED_DESCRIPTION_PATTERN_TMPL, dlcName, baseName);
        return StringHelper.matchesIgnoreCase(description, pattern);
    }

    /**
     * Verify link to the base game's PDP is visible and has the expected label
     * (i.e. the name of the base game)
     *
     * @return true if link visible with matching label, false otherwise
     */
    public boolean verifyBaseGamePDPLink() {
        String linkLabel = waitForElementVisible(BASE_PDP_LINK_LOCATOR).getText();
        return linkLabel.equalsIgnoreCase(baseName);
    }

    /**
     * Click the base game PDP link
     */
    public void clickBaseGamePDPLink() {
        waitForElementClickable(BASE_PDP_LINK_LOCATOR).click();
    }

    /**
     * Click the 'Continue Purchase' CTA.
     */
    public void clickContinuePurchaseCTA() {
        waitForElementVisible(CONTINUE_PURCHASE_CTA_LOCATOR).click();
    }
}