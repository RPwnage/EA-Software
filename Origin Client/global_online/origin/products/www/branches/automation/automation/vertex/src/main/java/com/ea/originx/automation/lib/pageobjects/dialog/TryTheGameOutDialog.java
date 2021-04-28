package com.ea.originx.automation.lib.pageobjects.dialog;

import com.ea.originx.automation.lib.pageobjects.template.Dialog;
import org.openqa.selenium.By;
import org.openqa.selenium.WebDriver;
import org.openqa.selenium.WebElement;

/**
 * Represents the 'Try out the game' Dialog that appears after clicking 'Try it
 * First' button in the GDP page
 *
 * @author cbalea
 * @author cdeaconu
 */
public class TryTheGameOutDialog extends Dialog {

    private static final By DIALOG_LOCATOR = By.cssSelector(".origin-store-gdp-trials");
    private static final By TITLE_LOCATOR = By.cssSelector(".otkmodal-header .otkmodal-title");
    public static final String[] DIALOG_TITLE_KEYWORDS = {"try", "it"};
    
    // large game card
    private static final String LARGE_GAME_CARD_CSS = ".origin-store-game-largetile";
    private final String LARGE_GAME_CARD_CSS_TMPL = LARGE_GAME_CARD_CSS + "[ocd-path*='%s']";
    private static final String ADD_GAME_TO_LIBRARY_CTA_CSS = "origin-cta-directacquisition";
    private static final By ADD_GAME_TO_LIBRARY_CTA_LOCATOR = By.cssSelector(ADD_GAME_TO_LIBRARY_CTA_CSS + " a");
    private final By ADD_TO_GAME_LIBRARY_MODAL_BUTTON = By.cssSelector( LARGE_GAME_CARD_CSS + " origin-cta-directacquisition a");
    private static final By LARGE_GAME_CARD_PACKART_LOCATOR = By.cssSelector(LARGE_GAME_CARD_CSS + "-packart");
    private static final By LARGE_GAME_CARD_TITLE_LOCATOR = By.cssSelector(LARGE_GAME_CARD_CSS + "-content > h1");
    private static final By LARGE_GAME_CARD_SUB_TITLE_LOCATOR = By.cssSelector(LARGE_GAME_CARD_CSS + "-subtitle");
    private static final By LARGE_GAME_CARD_DESCRIPTION_LOCATOR = By.cssSelector(LARGE_GAME_CARD_CSS + "-description");
    
    // footer
    private static final By CLOSE_BUTTON_LOCATOR = By.cssSelector(".otkmodal-footer .otkbtn-primary");
    
    /**
     * Constructor
     *
     * @param driver Selenium WebDriver
     */
    public TryTheGameOutDialog(WebDriver driver) {
        super(driver, DIALOG_LOCATOR, TITLE_LOCATOR, CLOSE_BUTTON_LOCATOR);
    }
    
    /**
     * Get large game card by version
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return element as By, null if not found
     */
    private By getLargeGameCardSection(String version){
        return By.cssSelector(String.format(LARGE_GAME_CARD_CSS_TMPL, version));
    }
    
    /**
     * Verify a specific version of a large game card section is visible
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return true if the large game card section is visible, false otherwise
     */
    public boolean verifyLargeGameCardSectionVisible(String version){
        return waitIsElementVisible(getLargeGameCardSection(version));
    }
    
    /**
     * Verify a specific version of a large game card has a title visible
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return true if the large game card title is visible and not empty,
     * false otherwise
     */
    public boolean verifyLargeGameCardTitleVisible(String version){
        WebElement title = waitForChildElementVisible(getLargeGameCardSection(version), LARGE_GAME_CARD_TITLE_LOCATOR);
        return title != null && !title.getText().isEmpty();
    }
    
    /**
     * Verify a specific version of a large game card has a sub-title visible
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return true if the large game card sub-title is visible and not
     * empty, false otherwise
     */
    public boolean verifyLargeGameCardSubTitleVisible(String version){
        WebElement subTitle = waitForChildElementVisible(getLargeGameCardSection(version), LARGE_GAME_CARD_SUB_TITLE_LOCATOR);
        return subTitle != null && !subTitle.getText().isEmpty();
    }
    
    /**
     * Verify a specific version of a large game card has a description visible
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return true if the large game card description is visible and not
     * empty, false otherwise
     */
    public boolean verifyLargeGameCardDescriptionVisible(String version){
        WebElement description = waitForChildElementVisible(getLargeGameCardSection(version), LARGE_GAME_CARD_DESCRIPTION_LOCATOR);
        return description != null && !description.getText().isEmpty();
    }
    
    /**
     * Verify a specific version of a large game card has a pack art visible
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return true if the large game card pack art is visible, false
     * otherwise
     */
    public boolean verifyLargeGameCardsPackArtVisible(String version){
        WebElement packArt = waitForChildElementVisible(getLargeGameCardSection(version), LARGE_GAME_CARD_PACKART_LOCATOR);
        return packArt != null;
    }
    
    /**
     * Verify a specific version of a large game card has a 'Add to game
     * library' CTA visible
     *
     * @param version a large game card section of an entitlement can have
     * multiple versions (demo, trial..)
     * @return true if the large game card has 'Add to game Library' CTA
     * visible, false otherwise
     */
    public boolean verifyLargeGameCardsAddToGameLibraryCTAVisible(String version){
        WebElement addGameToLibraryCTA = waitForChildElementVisible(getLargeGameCardSection(version), ADD_GAME_TO_LIBRARY_CTA_LOCATOR);
        return addGameToLibraryCTA != null;
    }
    
    /**
     * Clicks the 'Add to Game Library' Button
     *
     */
    public void clickAddGameToLibraryCTA() {
        waitForElementClickable(ADD_TO_GAME_LIBRARY_MODAL_BUTTON).click();
    }
    
    /**
     * Get the Offer Id of the trial entitlement
     *
     * @return offer id
     */
    public String getTrialOfferId() {
        return waitForElementVisible(By.cssSelector(ADD_GAME_TO_LIBRARY_CTA_CSS)).getAttribute("offer-id");
    }
}