package com.ea.originx.automation.lib.pageobjects.account;

import com.ea.vx.originclient.account.UserAccount;
import java.lang.invoke.MethodHandles;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Privacy Settings' page ('Account Settings' page with the
 * 'Privacy Settings' section open)
 *
 * @author palui
 */
public class PrivacySettingsPage extends AccountSettingsPage {

    /**
     * Privacy settings enum
     */
    public enum PrivacySetting {
        EVERYONE(0, "Everyone"),
        FRIENDS(1, "Friends"),
        FRIENDS_OF_FRIENDS(2, "Friends of Friends"),
        NO_ONE(3, "No One");
        private final int value;
        private final String label;

        PrivacySetting(int value, String label) {
            this.value = value;
            this.label = label;
        }

        @Override
        public String toString() {
            return label;
        }

    }

    protected static final By PROFILE_VISIBILITY_LOCATOR = By.id("profileVisibility");
    protected static final By DROP_DOWN_ITEMS_LOCATOR = By.cssSelector(".drop-down-item");
    protected static final By CURRENT_SETTING_LOCATOR = By.cssSelector("#profileVisibility > div > span");
    protected static final By SAVE_BUTTON_LOCATOR = By.id("updatebtn");
    protected static final By BLOCKED_USERS_LIST_LOCATOR = By.id("blockedUsers");
    protected static final By BLOCKED_USERS_LOCATOR = By.cssSelector("#blockedUsers .fleft > span");

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public PrivacySettingsPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Privacy Settings' section of the 'Account Settings' page is
     * open.
     *
     * @return true if open, false otherwise
     */
    public boolean verifyPrivacySettingsSectionReached() {
        return verifySectionReached(NavLinkType.PRIVACY_SETTINGS) && waitIsElementVisible(PROFILE_VISIBILITY_LOCATOR);
    }

    /**
     * Set the privacy setting to given setting and save.
     *
     * @param setting privacy setting to set to
     */
    public void setPrivacySetting(PrivacySetting setting) {
        // This is a selector, but is hidden, so we have to click on it manually
        waitForElementClickable(PROFILE_VISIBILITY_LOCATOR).click();
        waitForAllElementsVisible(DROP_DOWN_ITEMS_LOCATOR).get(setting.value).click();
        clickUpdateButton();
    }

    /**
     * Get the currently selected privacy setting.
     *
     * @return The currently selected privacy setting
     */
    public String getPrivacyLabel() {
        return waitForElementVisible(CURRENT_SETTING_LOCATOR).getText();
    }

    /**
     * Verify the current privacy settings match the given setting.
     *
     * @param setting The expected privacy setting
     * @return true if the current privacy setting matches the
     * expected privacy setting
     */
    public boolean verifyPrivacySettingMatches(PrivacySetting setting) {
        return getPrivacyLabel().equalsIgnoreCase(setting.label);
    }
    
    /**
     * Verifies the given user is on the block list.
     * 
     * @param userName Name of the account to verify is on the block list
     * @return true is user is on the list, false otherwise
     */
    public boolean verifyUserOnBlockList(String userName) {
        waitForElementVisible(BLOCKED_USERS_LIST_LOCATOR);
        if(waitIsElementPresent(BLOCKED_USERS_LOCATOR,1)) {
            List<WebElement> blockedUsers = driver.findElements(BLOCKED_USERS_LOCATOR);
            if(getElementWithText(blockedUsers, userName) != null){
                return true;
            }
        }
        return false;
    }
    
    /**
     * Remove specified user from the 'Blocked User' list
     * 
     * @param user User account for the user to be removed from the block list
     * @return true if user is removed, false otherwise
     */
    public boolean removeUserFromBlockList(UserAccount user) {
        if(verifyUserOnBlockList(user.getUsername())) {
            waitForElementClickable(By.id(user.getNucleusUserId())).click();
            sleep(500); // delay to allow the page to refresh before attempting to click the button to save changes
            clickUpdateButton();
            return true;
        }
        return false;
    }

    /**
     * Click the 'Update' button to save changes to the settings.
     */
    public void clickUpdateButton(){
        scrollToElement(waitForElementVisible(SAVE_BUTTON_LOCATOR)); // scroll needed because the update button is at the bottom of the page
        waitForElementClickable(SAVE_BUTTON_LOCATOR).click();
    }
}