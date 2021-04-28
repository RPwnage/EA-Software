package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.originx.automation.lib.pageobjects.dialog.EditBasicInfoDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.EditRegionalInformationDialog;
import java.lang.invoke.MethodHandles;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.LanguageInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'About Me' page ('Account Settings' page with the (default)
 * 'About Me' section open)
 *
 * @author palui
 */
public class AboutMePage extends AccountSettingsPage {

    private final By EDIT_BASIC_INFORMATION_LNK = By.id("editBasicInformation");
    private final By EDIT_REGIONAL_SETTINGS_LNK = By.id("editRegionSetting");
    private final By ORIGIN_ID_TEXT = By.id("origin_username");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public AboutMePage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'About Me' section of the 'Account Settings' page is open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyAboutMeSectionReached() {
        return verifySectionReached(NavLinkType.ABOUT_ME);
    }

    /**
     * Changes the user account's OriginID to {@code newID}
     *
     * @param newID The new ID to change to
     */
    public void changeOriginId(String newID) {
        navigateToIndexPage();
        waitForElementClickable(EDIT_BASIC_INFORMATION_LNK).click();
        answerSecurityQuestion();
        final EditBasicInfoDialog editBasicInfoDialog = new EditBasicInfoDialog(driver);
        editBasicInfoDialog.enterNewID(newID);
        editBasicInfoDialog.save();
    }

    /**
     * Changes the user account's country to the given country
     *
     * @param countryEnum The countryName to change to
     */
    public void changeCountryName(CountryInfo.CountryEnum countryEnum) {
        navigateToIndexPage();
        waitForElementClickable(EDIT_REGIONAL_SETTINGS_LNK).click();
        EditRegionalInformationDialog editRegionalInformationDialog = new EditRegionalInformationDialog(driver);
        editRegionalInformationDialog.selectCountry(countryEnum.getCountryCode());
    }

    /**
     * Changes the user account's language to the given language
     *
     * @param languageEnum The countryName to change to
     */
    public void changeLanguageName(LanguageInfo.LanguageEnum languageEnum) {
        navigateToIndexPage();
        waitForElementClickable(EDIT_REGIONAL_SETTINGS_LNK).click();
        EditRegionalInformationDialog editRegionalInformationDialog = new EditRegionalInformationDialog(driver);
        editRegionalInformationDialog.selectLanguage(languageEnum.getCountryCode(),languageEnum.getLanguageCode());
    }

    /**
     * Verifies that the Origin ID that appears on the page equals {@code id}
     *
     * @param id The id to match against
     * @return True if the Origin ID equals {@code id}
     */
    public boolean verifyOriginId(String id) {
        return waitForElementVisible(ORIGIN_ID_TEXT).getText().equals(id);
    }
}
