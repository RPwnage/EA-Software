package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;

/**
 * Component object class for the 'Remove From Library' dialog.
 *
 * @author glivingstone
 */
public final class RemoveFromLibrary extends Dialog {

    protected static final By REMOVE_FROM_LIBRARY_LOCATOR = By.cssSelector(".otkmodal-content .otkmodal-body .prompt-content .otkbtn-command .otkicon-remove");
    protected static final String RELEASE_NOTES_DIALOG_TITLE = "Remove From Library";
    protected static final By REMOVE_GAME_BUTTON = By.cssSelector("origin-dialog-content-commandbuttons button");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public RemoveFromLibrary(WebDriver driver) {
        super(driver);
    }
    
    /**
     * Click on the icon to remove the game from the library.
     */
    public void confirmRemoval() {
        clickOnElement(waitForElementVisible(REMOVE_FROM_LIBRARY_LOCATOR));
    }
    
    /**
     * Verify that 'Remove only this game from library' button is visible
     * 
     * @return true if the button is visible, false otherwise
     */
    public boolean verifyRemoveGameButtonVisible(){
        return waitIsElementVisible(REMOVE_GAME_BUTTON);
    }
}