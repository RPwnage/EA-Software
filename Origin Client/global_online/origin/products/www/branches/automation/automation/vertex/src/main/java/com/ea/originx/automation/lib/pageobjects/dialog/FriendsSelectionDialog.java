package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.By;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;
import org.openqa.selenium.remote.RemoteWebDriver;
import javax.annotation.Nullable;
import java.util.List;
import java.util.Optional;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Represents the dialog where you choose who to send your gift to after
 * selecting 'Gift This Game' from a game's PDP.
 *
 * @author jmittertreiner
 */
public final class FriendsSelectionDialog extends Dialog {

    private static final String FRIENDS_LIST_CSS = "origin-store-gift-friendslist";
    private static final By DIALOG_LOCATOR = By.cssSelector(FRIENDS_LIST_CSS);
    private static final By TITLE_LOCATOR = By.cssSelector(FRIENDS_LIST_CSS + " .otkmodal-header h4");
    private static final By SEARCH_BOX_LOCATOR = By.cssSelector("#filter-by-text");
    private static final By NEXT_BUTTON_LOCATOR = By.cssSelector("div.otkmodal-footer > button.otkbtn.otkbtn-primary");
    private static final By FRIENDS_LIST_LOCATOR = By.cssSelector(FRIENDS_LIST_CSS + " div.origin-store-gift-friendslist-wrapper div.origin-store-gift-friendslist-friends");
    private static final By NO_USER_FOUND_LOCATOR = By.cssSelector(FRIENDS_LIST_CSS + " div.origin-store-gift-friendslist-wrapper div.origin-store-gift-friendslist-nofriends div.page-message h2 span");
    protected static final String[] EXPECTED_NO_USER_FOUND_KEYWORDS = {"NO", "RESULT", "FOUND"};
    protected static final String[] EXPECTED_TITLE_KEYWORDS = {"SEND", "GIFT"};

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public FriendsSelectionDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR);
    }

    /**
     * Enter text into the search box.
     *
     * @param text The text to enter into the search box
     */
    public void searchFor(String text) {
        waitForElementVisible(SEARCH_BOX_LOCATOR).sendKeys(text);
    }

    /**
     * Clear the search box.
     */
    public void clearSearch() {
        waitForElementVisible(SEARCH_BOX_LOCATOR).clear();
    }

    /**
     * Click the 'Next' button.
     */
    public void clickNext() {
        waitForElementClickable(NEXT_BUTTON_LOCATOR).click();
    }

    /**
     * Verify if the 'Next' button is enabled.
     *
     * @return true if enabled, false otherwise, or throws TimeoutException if
     * button not found
     */
    public boolean verifyNextButtonEnabled() {
        return waitForElementVisible(NEXT_BUTTON_LOCATOR).isEnabled();
    }

    /**
     * Verify if the 'Next' button is enabled.
     *
     * @return true if it is enabled, false otherwise
     */
    public boolean isNextButtonEnabled() {
        return isElementEnabled(NEXT_BUTTON_LOCATOR);
    }

    /**
     * Verify dialog title matches the expected keywords.
     *
     * @return true if title matches, false otherwise, or throw TimeoutException
     * if button not found
     */
    public boolean verifyFriendsDialogTitle() {
        return waitIsElementVisible(TITLE_LOCATOR) && verifyTitleContainsIgnoreCase(EXPECTED_TITLE_KEYWORDS);
    }

    /**
     * Verify the search bar exists in the dialog.
     *
     * @return true if it is displayed, false otherwise
     */
    public boolean verifySearchBarExists() {
        return isElementVisible(SEARCH_BOX_LOCATOR);
    }

    /**
     * Verify the 'No User Found' message exists in the dialog.
     *
     * @return true if message visible, false otherwise
     */
    public boolean verifyNoUserFoundExists() {
        return waitIsElementVisible(NO_USER_FOUND_LOCATOR) && StringHelper.containsAnyIgnoreCase(waitForElementVisible(NO_USER_FOUND_LOCATOR).getText(), EXPECTED_NO_USER_FOUND_KEYWORDS);
    }

    /**
     * The eligibility of a person to receive a game.ON_WISHLIST and ELIGIBLE
     * are both eligible to be gifted to, while ALREADY_OWNS and INELIGIBLE are
     * not eligible able to be gifted to.
     */
    public enum Eligibility {
        ON_WISHLIST, ALREADY_OWNS, ELIGIBLE, INELIGIBLE
    }

    /**
     * Verify the recipients list exists in the dialog.
     *
     * @return true if it is displayed, false otherwise
     */
    public boolean verifyRecipientsListExists() {
        List<Recipient> recipients = getAllRecipients();
        return !recipients.isEmpty() && isElementVisible(FRIENDS_LIST_LOCATOR);
    }

    /**
     * Get all the recipients that are displayed in the 'Recipient Suggestions'.
     *
     * @return The list of displayed recipient suggestions
     */
    public List<Recipient> getAllRecipients() {
        return Recipient.getRecipients(driver).collect(Collectors.toList());
    }

    /**
     * Get all the recipients that are displayed in the 'Recipient Suggestions'
     * by a given user account.
     *
     * @return The list of displayed recipient suggestions
     */
    public Recipient getRecipient(UserAccount account) {
        return getAllRecipients().stream()
                .filter(r -> {
                    try {
                        return r.getUsername().equals(account.getUsername());
                    } catch (StaleElementReferenceException e) {
                        return false;
                    }
                })
                .findFirst()
                .orElseThrow(() -> new RuntimeException("There are no recipients of username " + account.getUsername()));
    }

    /**
     * Get all the eligible recipients.
     *
     * @return The list of all displayed and eligible recipients
     */
    public List<Recipient> getAllEligibleRecipients() {
        final Predicate<Recipient> isEligible = r -> {
            Eligibility e = r.getEligibility();
            return e == Eligibility.ELIGIBLE || e == Eligibility.ON_WISHLIST;
        };
        return Recipient.getRecipients(driver).filter(isEligible).collect(Collectors.toList());
    }

    /**
     * Get all the ineligible recipients.
     *
     * @return The list of all displayed and ineligible recipients.
     */
    public List<Recipient> getAllIneligibleRecipients() {
        final Predicate<Recipient> isIneligible = recipient -> {
            Eligibility e = recipient.getEligibility();
            return e == Eligibility.ALREADY_OWNS || e == Eligibility.INELIGIBLE;
        };
        return Recipient.getRecipients(driver).filter(isIneligible).collect(Collectors.toList());
    }

    /**
     * Get all the recipients that have the game on their wishlist.
     *
     * @return The list of all recipients which have the game on their wishlist
     */
    public List<Recipient> getAllOnWishlistRecipients() {
        final Predicate<Recipient> isOnWishlist = recipient -> {
            Eligibility e = recipient.getEligibility();
            return e == Eligibility.ON_WISHLIST;
        };
        return Recipient.getRecipients(driver).filter(isOnWishlist).collect(Collectors.toList());
    }

    /**
     * Get a recipient entry from the friends list.<br>
     * Note: Retries if 'Stale Element Reference' exception encountered
     *
     * @param username Username to match
     * @return Recipient entry with the given username, or null if not found
     */
    public @Nullable
    Recipient getRecipientIfPresent(String username) {
        List<Recipient> allRecipients;
        int numRetries = 2;
        for (int i = 0; i < numRetries; i++) {
            allRecipients = getAllRecipients();
            for (Recipient recipientEntry : allRecipients) {
                try {
                    if (recipientEntry.getUsername().equals(username)) {
                        return recipientEntry;
                    }
                    // Changing friends list can cause 'Stale Element Reference' exception
                } catch (StaleElementReferenceException e) {
                    sleep(2000); // sleep 2 sec and re-try inner loop
                    break;
                }
            }
        }
        return null;
    }

    /**
     * Verify the recipient list is sorted alphabetically.
     *
     * @return true if the recipient list is in alphabetical order,
     * false otherwise
     */
    public boolean verifyRecipientsSorted() {
        List<String> usernames = getAllRecipients().stream()
                .map(Recipient::getUsername).collect(Collectors.toList());
        return TestScriptHelper.verifyListSortedIgnoreCase(usernames,false);
    }

    /**
     * Check presence of a user in the friends list.
     *
     * @param username Username to look for in the friends list
     * @return true if user present, false otherwise
     */
    public boolean isRecipientPresent(String username) {
        return getRecipientIfPresent(username) != null;
    }

    /**
     * Get a recipient from the friends list.
     *
     * @param username Username of the recipient to get from the friends list
     * @return Recipient entry with the given username, or null if not found
     */
    public Recipient getRecipient(String username) {
        if (Waits.pollingWait(() -> getRecipientIfPresent(username) != null)) {
            return getRecipientIfPresent(username);
        }
        throw new RuntimeException(String.format("Cannot find recipient entry named %s in the friends list", username));
    }

    /**
     * Select a recipient from the friends list.
     *
     * @param username Username of recipient to select
     */
    public Recipient selectRecipient(String username) {
        Recipient recipient = getRecipient(username);
        recipient.select();
        return recipient;
    }

    /**
     * Verify recipient is eligible for gifting.
     *
     * @param username Name of recipient to check eligibility of
     * @return true if user is eligible for gifting, false otherwise
     */
    public boolean recipientIsEligible(String username) {
        Recipient recipient = getRecipient(username);
        return recipient.getEligibility() == Eligibility.ELIGIBLE;
    }

    /**
     * Verify the recipient is ineligible for gifting.
     *
     * @param username Name of recipient to check ineligibility of
     * @return true if user ineligible to be gifted this offer, false otherwise
     */
    public boolean recipientIsIneligible(String username) {
        Recipient recipient = getRecipient(username);
        return recipient.getEligibility() == Eligibility.INELIGIBLE;
    }

    /**
     * Created to differentiate from recipientAlreadyOwns(String username) by
     * ONLY checking if the user already owns the entitlement and not if it's
     * ineligible as well.
     *
     * @param username Name of recipient to check eligibility of
     * @return true if user is ineligible to be gifted this offer due to already
     * owning the offer, false otherwise
     */
    public boolean recipientJustAlreadyOwns(String username) {
        Recipient recipient = getRecipient(username);
        return recipient.getEligibility() == Eligibility.ALREADY_OWNS;
    }

    /**
     * Verify the given user can be gifted to.
     *
     * @param username The name of the user to gift to
     * @return true if the user can be gifted to, false otherwise
     */
    public boolean canGiftTo(String username) {
        Eligibility eligibility = getRecipient(username).getEligibility();
        return eligibility == Eligibility.ELIGIBLE || eligibility == Eligibility.ON_WISHLIST;
    }

    /**
     * Verify the given user can be gifted to.
     *
     * @param account The user account to gift to
     * @return true if the user can be gifted to, false otherwise
     */
    public boolean canGiftTo(UserAccount account) {
        return canGiftTo(account.getUsername());
    }

    /**
     * Check if a given user already owns the game.
     *
     * @param username Username of recipient to check eligibility of
     * @return true if user is already owns the game, false otherwise
     */
    public boolean recipientAlreadyOwns(String username) {
        Recipient recipient = getRecipient(username);
        return recipient.getEligibility() == Eligibility.ALREADY_OWNS || recipient.getEligibility() == Eligibility.INELIGIBLE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Methods using UserAccount
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Check presence of a user in the friends list.
     *
     * @param userAccount Account containing name to match
     * @return true if user present, false otherwise
     */
    public boolean isRecipientPresent(UserAccount userAccount) {
        return isRecipientPresent(userAccount.getUsername());
    }

    /**
     * Selects a user account from the 'Selection' dialog.
     *
     * @param userAccount User account containing name to select
     */
    public void selectRecipient(UserAccount userAccount) {
        selectRecipient(userAccount.getUsername());
    }

    /**
     * Scroll to the bottom of the list of friends and trigger the lazy load.
     */
    public void scrollToBottom() {
        List<Recipient> allRecipients = getAllRecipients();
        allRecipients.get(allRecipients.size() - 1).scrollTo();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Recipient class
    ////////////////////////////////////////////////////////////////////////////
    /**
     * Represents a possible recipient on the selection dialog. This class wraps
     * an element found by RECIPIENTS_LOCATOR and encapsulates functionality
     */
    public static class Recipient {

        private final WebElement rootElement;
        private final WebDriver driver;
        private static final By RECIPIENTS_LOCATOR = By.cssSelector("div.origin-store-gift-friendslist-friends-wrapper > div > ul > li");
        private final By USERNAME_LOCATOR = By.cssSelector("div.origin-store-gift-friendtile-userinfo > a.origin-telemetry-store-gift-friendtile-profilelink");
        private final By REALNAME_LOCATOR = By.cssSelector("div.origin-store-gift-friendtile-userinfo > h3:nth-of-type(2)");
        private final By TILE_LOCATOR = By.cssSelector("origin-store-gift-friendtile > div");
        private final By ELIGIBILITY_LOCATOR = By.cssSelector("div.origin-store-gift-friendtile-eligibility > span");

        private static final String ACTION_BUTTON_CSS = ".origin-store-gift-friendtile-eligibility .origin-store-gift-friendtile-action";
        private static final By ACTION_BUTTON_LOCATOR = By.cssSelector(ACTION_BUTTON_CSS);
        private static final By PLUS_BUTTON_LOCATOR = By.cssSelector(ACTION_BUTTON_CSS + ".otkicon-add");
        private static final By CHECK_CIRCLE_BUTTON_LOCATOR = By.cssSelector(ACTION_BUTTON_CSS + ".otkicon-checkcircle");
        private static final By STATUS_MESSAGE_LOCATOR = By.cssSelector(".origin-store-gift-friendtile-eligibility > span");

        /**
         * Constructor
         *
         * @param rootElement The element to wrap as a 'Recipient'
         */
        private Recipient(WebElement rootElement, WebDriver driver) {
            this.rootElement = rootElement;
            this.driver = driver;
        }

        /**
         * Gets a stream of the expected recipients.
         *
         * @return A stream of the suggested recipients
         */
        private static Stream<Recipient> getRecipients(WebDriver driver) {
            return driver.findElements(Recipient.RECIPIENTS_LOCATOR).stream()
                    .map(we -> new Recipient(we, driver));
        }

        /**
         * Get the username of the recipient.
         *
         * @return The username of the recipient
         */
        public String getUsername() {
            return rootElement.findElement(USERNAME_LOCATOR).getText();
        }

        /**
         * Get the real name of the recipient.
         *
         * @return The real name of the recipient
         */
        public String getRealname() {
            return rootElement.findElement(REALNAME_LOCATOR).getAttribute("textContent");
        }

        /**
         * Get the eligibility of a recipient.
         *
         * @return The eligibility of a recipient
         */
        public Eligibility getEligibility() {
            final WebElement statusElement = rootElement.findElement(ELIGIBILITY_LOCATOR);
            String eligibilityText = statusElement.getAttribute("textContent").trim();

            // If there is no additional information, the user is eligible
            if (eligibilityText.isEmpty()) {
                return Eligibility.ELIGIBLE;
            }
            if (StringHelper.containsIgnoreCase(eligibilityText, "wishlist")) {
                return Eligibility.ON_WISHLIST;
            } else if (StringHelper.containsIgnoreCase(eligibilityText, "already")) {
                return Eligibility.ALREADY_OWNS;
            } else if (StringHelper.containsIgnoreCase(eligibilityText, "ineligible")) {
                return Eligibility.INELIGIBLE;
            } else if (StringHelper.containsIgnoreCase(eligibilityText, "determine")) {
                // There is also a nonspec "Cannot determine", which makes the account
                // ineligible to gift to
                return Eligibility.INELIGIBLE;
            } else {
                throw new RuntimeException("Could not determine eligibility of user " + getUsername());
            }
        }

        /**
         * Select this recipient to receive the gift.
         */
        public void select() {
            // Due to animations, we may click the tile too soon which will do nothing.
            // Try a couple of times in case we didn't get it the first time.
            Waits.pollingWait(() -> {
                rootElement.click();
                return Waits.pollingWait(this::isSelected);
            });
        }

        /**
         * Click the button of a recipient (with 'Plus' for unselected or 'Tick'
         * for selected) that toggles the selection status of a recipient entry.
         */
        public void toggleSection() {
            rootElement.click();
        }

        /**
         * Check if the button with the 'Plus' sign is visible.
         *
         * @return true if visible, false otherwise
         */
        public boolean isPlusButtonVisible() {
            return rootElement.findElements(PLUS_BUTTON_LOCATOR).size() > 0;
        }

        /**
         * Check if the button with the 'Tick' sign is visible.
         *
         * @return true if visible, false otherwise
         */
        public boolean isCheckCircleButtonVisible() {
            return rootElement.findElements(CHECK_CIRCLE_BUTTON_LOCATOR).size() > 0;
        }

        /**
         * Check if the user is selected.
         *
         * @return true if the user is selected, false otherwise
         */
        public boolean isSelected() {
            return rootElement.findElement(TILE_LOCATOR).getAttribute("class").contains("selected");
        }

        /**
         * Some eligibilities have an additional message describing the status.
         * e.g. If the user owns the game, there is a message "User already has
         * this item", or if it is on their wishlist, there is a message indicating
         * so.
         * @return The message if it exists, null otherwise
         */
        public Optional<String> getStatusDetails() {
            String msg = rootElement.findElement(STATUS_MESSAGE_LOCATOR)
                    .getAttribute("textContent");
            return msg == null || msg.isEmpty()
                    ? Optional.empty()
                    : Optional.of(msg);
        }

        /**
         * Scrolls to the recipient.
         */
        void scrollTo() {
            ((RemoteWebDriver) driver).executeScript("arguments[0].scrollIntoView()", rootElement);
        }
    }
}