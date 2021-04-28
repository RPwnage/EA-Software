package com.ea.originx.automation.lib.pageobjects.social;

import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Page object that represents the 'Chat Window' when it is minimized.
 *
 * @author glivingstone
 */
public class ChatWindowMinimized extends EAXVxSiteTemplate {

    protected By CHAT_WINDOW_MINIMIZED = By.cssSelector(".origin-social-chatwindow-area-minimized-button");

    protected By OFFLINE_INDICATOR = By.xpath("//i[contains(@class,'otkpresence otkpresence-offline')]");
    protected By ONLINE_INDICATOR = By.xpath("//i[contains(@class,'otkpresence otkpresence-online')]");
    protected By INGAME_INDICATOR = By.xpath("//i[contains(@class,'otkpresence otkpresence-ingame')]");
    protected By AWAY_INDICATOR = By.xpath("//i[contains(@class,'otkpresence otkpresence-away')]");

    public ChatWindowMinimized(WebDriver driver) {
        super(driver);
    }

    /**
     * Restore the chat window from being minimized to regular size.
     */
    public void restoreChatWindow() {
        waitForElementClickable(CHAT_WINDOW_MINIMIZED).click();
    }

    /**
     * Verify the chat window is minimized.
     *
     * @return true if the chat window is minimized, false otherwise
     */
    public boolean verifyMinimized() {
        return waitIsElementVisible(CHAT_WINDOW_MINIMIZED);
    }

    /**
     * Verify there is an indicator showing the user is offline on the minimized
     * chat window.
     *
     * @return true if the offline presence indicator exists, false otherwise.
     */
    public boolean verifyOfflineIndicator() {
        return driver.findElements(OFFLINE_INDICATOR).size() > 0;
    }
    
        /**
      * Verify there is an indicator showing the user is online on the minimized
      * chat window.
      *
      * @return true if the online presence indicator exists, false otherwise.
      */
     public boolean verifyOnlineIndicator() {
         return driver.findElements(ONLINE_INDICATOR).size() > 0;
     }
     
     /**
      * Verify there is an indicator showing the user is in game on the minimized
      * chat window.
      *
      * @return true if the in game presence indicator exists, false otherwise.
      */
     public boolean verifyInGameIndicator() {
         return driver.findElements(INGAME_INDICATOR).size() > 0;
     }
     
     /**
      * Verify there is an indicator showing the user is away on the minimized
      * chat window.
      *
      * @return true if the away presence indicator exists, false otherwise.
      */
     public boolean verifyAwayIndicator() {
         return driver.findElements(AWAY_INDICATOR).size() > 0;
     }
     
     /**
      * Verify there is an indicator showing the user's presence on the minimized
      * chat window and verify the username is visible 
      * @param username the name of the user we check 
      * @return true if the indicator and the username are visible, false otherwise
      */
     public boolean verifyNameAndPresenceDisplayed(String username) {
         boolean isNameDisplayed = driver.findElement(CHAT_WINDOW_MINIMIZED).getText().contains(username);
         boolean isPresenceDisplayed = verifyAwayIndicator() || verifyInGameIndicator() || verifyOfflineIndicator() || verifyOnlineIndicator();
         return isNameDisplayed && isPresenceDisplayed;
     }
}
