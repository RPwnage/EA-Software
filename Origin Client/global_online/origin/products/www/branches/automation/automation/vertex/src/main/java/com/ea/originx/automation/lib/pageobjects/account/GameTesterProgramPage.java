package com.ea.originx.automation.lib.pageobjects.account;

import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;

/**
 * Represent the 'Game Tester Program' settings page ('Account Settings' page
 * with the 'Game Tester Program' section open)
 *
 * @author palui
 */
public class GameTesterProgramPage extends AccountSettingsPage {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public GameTesterProgramPage(WebDriver driver) {
        super(driver);
    }

    /**
     * Verify the 'Game Tester Program' section of the 'Account Settings' page
     * is open
     *
     * @return true if open, false otherwise
     */
    public boolean verifyGameTesterProgramSectionReached() {
        return verifySectionReached(NavLinkType.GAME_TESTER_PROGRAM);
    }
}
