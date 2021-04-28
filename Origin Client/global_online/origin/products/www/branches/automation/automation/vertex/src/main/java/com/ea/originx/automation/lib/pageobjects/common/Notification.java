package com.ea.originx.automation.lib.pageobjects.common;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.pageobjects.template.OAQtSiteTemplate;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.apache.commons.lang3.tuple.Pair;
import org.openqa.selenium.By;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents 'Notifications' in the Origin client.
 *
 * @author rchoi
 */
public class Notification extends EAXVxSiteTemplate {

    //URL For toasty window
    private static final String NOTIFICATION_URL = "qtwidget://Origin::UIToolkit::OriginNotificationDialog";
    private static final String OIG_NOTIFICATION_URL = "qtwidget://Origin::Engine::IGOQWidget";
    private static final int QT_CLIENT_TOAST_TIMEOUT = 15;

    public Notification(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify username does not exist in the title of the Toasty message.
     *
     * @param username The username to check for
     * @return true if the username is not in the messages
     */
    public boolean verifyUserNameNotInMessages(String username) {
        // If there are no message, and none arrive in the next few seconds, the
        // username is certainly not in the messages
        if (!Waits.pollingWait(this::isNotificationWindowPresent, 5000, 100, 0)) {
            return true;
        }
        return !verifyToastyNotificationTitle(username);
    }

    /**
     * Verify the 'Notification' window is present.
     *
     * @return true if the window which contains the notifications is present,
     * false otherwise
     */
    public boolean isNotificationWindowPresent() {
        return OAQtSiteTemplate.isWindowPresent(driver, NOTIFICATION_URL);
    }

    /**
     * Verify the 'OIG Notification' window is present.
     *
     * @return true if the window which contains the notifications is present,
     * false otherwise
     */
    public boolean isOigNotificationWindowPresent() {
        Waits.sleep(18000); // wait a while before checking
        return OAQtSiteTemplate.isWindowPresent(driver, OIG_NOTIFICATION_URL);
    }

    /**
     * Verify that the OIG toast notification message contains the keys required
     * to open and close OIG. For example,
     * <br><br> {@code verifyOIGToastMessageContainsKeys("Ctrl", "F5")}
     * <br><br>
     * will return true if the OIG toast notification contains both "Ctrl" and
     * "F5".
     *
     * @param keyCombination The keys to look for in the OIG toast notification
     * @return true if all the keys are present, false otherwise
     */
    public boolean verifyOIGToastMessageContainsKeys(Pair<String, String> keyCombination) {
        final By modifierKeyLocator = By.id("lblModifierKey");
        final By modifiedKeyLocator = By.id("lblUnmodifiedKey");
        WebDriver qtDriver = OriginClient.getInstance(driver).getQtWebDriver();
        // Store the window handle so we can restore it
        String windowHandle = qtDriver.getWindowHandle();
        try {
            Waits.waitForPageThatMatches(qtDriver, OIG_NOTIFICATION_URL, QT_CLIENT_TOAST_TIMEOUT);
        } catch (TimeoutException e) {
            return false;
        }
        String modifierKeyText = qtDriver.findElement(modifierKeyLocator).getText();
        String modifiedKeyText = qtDriver.findElement(modifiedKeyLocator).getText();
        qtDriver.switchTo().window(windowHandle);
        return keyCombination.getLeft().equalsIgnoreCase(modifierKeyText)
                && keyCombination.getRight().equalsIgnoreCase(modifiedKeyText);
    }

    /**
     * Waits for the OIG toast notification to close.
     */
    public void waitForOIGToastToClose() {
        Waits.pollingWait(() -> !isNotificationWindowPresent());
    }

    /**
     * Verify the title exists in the toasty message.
     *
     * @param title title of the toasty message
     * @return true if the title is not in the messages,
     * false otherwise
     */
    public boolean verifyToastyNotificationTitle(String title) {
        WebDriver qtDriver = OriginClient.getInstance(driver).getQtWebDriver();
        // Store the window handle so we can restore it
        String windowHandle = qtDriver.getWindowHandle();
        Waits.waitForPageThatMatches(qtDriver, NOTIFICATION_URL, QT_CLIENT_TOAST_TIMEOUT);
        final By toastTitleLocator = By.id("lblTitle");
         final boolean titleExists = qtDriver.findElements(toastTitleLocator).stream()
                .map(WebElement::getText)
                .anyMatch(t -> StringHelper.containsAnyIgnoreCase(t, title));
        // Restore the window
        qtDriver.switchTo().window(windowHandle);
        return titleExists;
    }
}