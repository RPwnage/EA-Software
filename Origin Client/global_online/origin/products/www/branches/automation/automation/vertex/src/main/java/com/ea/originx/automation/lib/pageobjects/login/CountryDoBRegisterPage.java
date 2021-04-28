package com.ea.originx.automation.lib.pageobjects.login;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.support.ui.Select;
import java.util.List;
import java.util.stream.Collectors;

/**
 * Represents the 'Register' page that has fields for 'Country' and 'Date of Birth'.
 *
 * @author mkalaivanan
 * @author jmittertreiner
 */
public class CountryDoBRegisterPage extends EAXVxSiteTemplate {

    private static final Logger _log = LogManager.getLogger(CountryDoBRegisterPage.class);

    protected static final By COUNTRY_DROPDOWN_LOCATOR = By.cssSelector("#clientreg_country-selctrl");
    protected static final By MONTH_DROPDOWN_LOCATOR = By.cssSelector("#clientreg_dobmonth-selctrl");
    protected static final By DAY_DROPDOWN_LOCATOR = By.cssSelector("#clientreg_dobday-selctrl");
    protected static final By YEAR_DROPDOWN_LOCATOR = By.cssSelector("#clientreg_dobyear-selctrl");
    protected static final By COUNTRY_DOB_CONTINUE_BUTTON_LOCATOR = By.cssSelector("#countryDobNextBtn");
    protected static final By TOS_CHECKBOX_LOCATOR = By.cssSelector("#readAccept");
    protected static final By MOBILE_INFO_LOCATOR = By.cssSelector("#mobileInfo");
    protected static final By IPIN_LOCATOR = By.cssSelector("#iPin");
    protected static final By VERIFY_BTN_LOCATOR = By.cssSelector("#southKoreaVerifyBtn");

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public CountryDoBRegisterPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Selects the given country from the 'Country' dropdown.
     *
     * @param country The country to select
     */
    public void selectCountry(String country) {
        new Select(waitForElementPresent(COUNTRY_DROPDOWN_LOCATOR))
                .selectByVisibleText(country);
    }

    /**
     * Selects the given month from the 'Month' dropdown.
     *
     * @param month The month to select
     */
    public void selectMonth(String month) {
        new Select(waitForElementPresent(MONTH_DROPDOWN_LOCATOR))
                .selectByVisibleText(month);
    }

    /**
     * Selects the given day from the 'Day' dropdown
     *
     * @param day The day to select
     */
    public void selectDay(String day) {
        new Select(waitForElementPresent(DAY_DROPDOWN_LOCATOR))
                .selectByVisibleText(day);
    }

    /**
     * Selects the given year from the 'Year' dropdown.
     *
     * @param year The year to select
     */
    public void selectYear(String year) {
        new Select(waitForElementPresent(YEAR_DROPDOWN_LOCATOR))
                .selectByVisibleText(year);
    }

    /**
     * Enter the given date of birth in the 'Date of Birth' field.
     *
     * @param month The month the user was born
     * @param day The day the user was born
     * @param year The year the user was born
     */
    public void enterDoB(String month, String day, String year) {
        selectMonth(month);
        sleep(2000);// adding this sleep to reduce the number of ajax requests at time in a page
        selectDay(day);
        selectYear(year);
    }

    /**
     * Start registering the user with 'Date of Birth' and 'Country'
     * information.
     *
     * @param userAccount The account to register as
     */
    public void inputUserDOBRegistrationDetails(UserAccount userAccount) {
        selectCountry(userAccount.getCountry());
        enterDoB(userAccount.getBirthMonth(), userAccount.getBirthDay(), userAccount.getBirthYear());
    }

    /**
     * Accept (check) the 'Terms of Service'.
     */
    public void acceptToS() {
        setCheckbox(TOS_CHECKBOX_LOCATOR, true);
    }

    /**
     * Decline (uncheck) the 'Terms of Service'.
     */
    public void declineToS() {
        setCheckbox(TOS_CHECKBOX_LOCATOR, false);
    }

    /**
     * Click on the 'Next' button on the 'Country DOB' section.
     *
     */
    public void clickOnCountryDOBNextButton() {
        _log.debug("clicking country dob next");
        forceClickElement(waitForElementClickable(COUNTRY_DOB_CONTINUE_BUTTON_LOCATOR));
    }

    /**
     * Complete user 'Date of Birth' and 'Country Information'.
     *
     * @param userAccount The user account to enter the information of
     */
    public void enterDoBCountryInformation(UserAccount userAccount) {
        inputUserDOBRegistrationDetails(userAccount);
        acceptToS();
        clickOnCountryDOBNextButton();
    }

    /**
     * Verify the 'Country' input field is visible.
     *
     * @return true if 'Country' dropdown is visible, false otherwise
     */
    public boolean verifyCountryInputVisible() {
        return waitIsElementPresent(COUNTRY_DROPDOWN_LOCATOR);
    }

    /**
     * Verify 'Date of Birth' input field is visible.
     *
     * @return true if 'Date Of Birth' is visible, false otherwise
     */
    public boolean verifyDateOfBirthVisible() {
        return waitIsElementPresent(MONTH_DROPDOWN_LOCATOR)
                && waitIsElementPresent(DAY_DROPDOWN_LOCATOR)
                && waitIsElementPresent(YEAR_DROPDOWN_LOCATOR);
    }

    /**
     * Verify if 'Continue' button is disabled
     *
     * @return true if 'Continue' button is disabled, false otherwise
     */
    public boolean verifyContinueButtonIsDisabled() {
        return waitForElementVisible(COUNTRY_DOB_CONTINUE_BUTTON_LOCATOR)
                .getAttribute("class")
                .contains("disabled");
    }

    /**
     * Get the current 'Day' option.
     *
     * @return Current 'Day' dropdown value, empty String otherwise
     */
    public String getDay() {
        return getSelectedDropDownValue(DAY_DROPDOWN_LOCATOR);
    }

    /**
     * Get the current 'Month' option
     *
     * @return Current 'Month' dropdown value, empty String otherwise
     */
    public String getMonth() {
        return getSelectedDropDownValue(MONTH_DROPDOWN_LOCATOR);
    }

    /**
     * Get the current 'Year' option.
     *
     * @return Current 'Year' dropdown value, empty String otherwise
     */
    public String getYear() {
        return getSelectedDropDownValue(YEAR_DROPDOWN_LOCATOR);
    }

    /**
     * Get the current 'Country' option
     *
     * @return Current 'Country' dropdown value, empty String otherwise
     */
    public String getCountry() {
        return getSelectedDropDownValue(COUNTRY_DROPDOWN_LOCATOR);
    }

    /**
     * Get all 'Country' options
     *
     * @return All possible countries
     */
    public List<String> getCountries() {
        return new Select(waitForElementPresent(COUNTRY_DROPDOWN_LOCATOR))
                .getOptions().stream()
                .map(WebElement::getText)
                .collect(Collectors.toList());
    }
    /**
     * Get the current selected dropdown value.
     *
     * @param locator The locator of the selector to search
     * @return The currently selected value or "" if there is nothing selected
     */
    private String getSelectedDropDownValue(By locator) {
        Select select = new Select(waitForElementPresent(locator));
        WebElement selected = select.getFirstSelectedOption();
        return selected == null ? "" : selected.getText().trim();
    }

    /**
     * Verifies that the page contains the elements needed for verification in
     * South Korea. The South Korean page is different and requests verification
     * by 'i-PIN' or 'Mobile Phone Information.
     *
     * @return true if the 'i-PIN' and 'Mobile Phone Information' options exist and
     * that the 'Confirm' button instead reads 'Verify Identity'
     */
    public boolean verifySouthKoreanRegistration() {
        boolean isMobilePhoneVisible = waitIsElementPresent(MOBILE_INFO_LOCATOR);
        boolean isiPinVisible = waitIsElementPresent(IPIN_LOCATOR);
        boolean isVerifyBtnCorrect = waitForElementPresent(VERIFY_BTN_LOCATOR)
                .getText()
                .trim()
                .equals("Verify Identity");
        return isMobilePhoneVisible && isiPinVisible && isVerifyBtnCorrect;
    }
}