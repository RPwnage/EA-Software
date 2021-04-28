package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeader;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.*;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.apache.commons.lang3.StringUtils;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Performance profiling test that handles hitting 23 pages in sequence
 * chrome_performance_profiling.xml was designed for running this script in
 * Chrome.
 *
 * NEEDS UPDATE TO GDP
 */
public class OAPerformanceProfiling extends EAXVxTestTemplate {

    private static final Logger _log = LogManager.getLogger(OAPerformanceProfiling.class);

    @Test(groups = {"browser", "int_only", "browser_only"})
    public void testImpl(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);

        // 1
        final WebDriver driver = startClientObject(context, client);
        System.out.println("Driver srouce" + driver.getCurrentUrl());
        //"Store-GGG",
        //                "Store-GGGPolicy"
        List<String> pagesToNavigatelist = Arrays.asList("Discover", "Search", "GameLibrary", "Store-Home", "Store-PDP",
                "Store-Gametime", "Store-OnTheHouse", "Store-Freegames", "Store-Download",
                "Store-DemosBetas", "Store-Deals", "Store-Bundle", "Store-Browse",
                "Store-Addon", "Store-About", "Store-OriginAccess", "Store-OriginAccessFAQ",
                "Store-OriginAccessTrials", "Store-OriginAccessVaults", "Settings", "SettingsPage");

        for (int i = 0; i < 10; i++) {
            logFlowPoint("Launch Origin and Login as: " + user.getUsername()); // 1
            logFlowPoint("Navigate to through list of pages"); //2
        }

        for (int i = 0; i < 10; ++i) {

            if (MacroLogin.startLogin(driver, user)) {
                logPass("User logged in successfully");
            } else {
                logFail("Could not log into Origin with the user.");
            }
            NavigationSidebar navBar = new NavigationSidebar(driver);
            String page;
            BrowseGamesPage browseGamesPage;
            StoreHomeTile storeHomeTile;
            GDPHeader gdpHeader;
            ArrayList<String> listofPagesFailedToLoad = new ArrayList<>();

            for (int j = 0; j < pagesToNavigatelist.size(); j++) {
                page = pagesToNavigatelist.get(j);
                try {
                    switch (j) {
                        case 0:
                            //discover/home page
                            if (!navBar.gotoDiscoverPage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;

                        case 1:
                            //Search
                            GlobalSearchResults searchResults = new GlobalSearchResults(driver);
                            searchResults.waitForPageToLoad();
                            searchResults.waitForJQueryAJAXComplete(10);

                            if (!searchResults.isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 2:
                            //GameLibrary
                            GameLibrary gameLibrary = navBar.gotoGameLibrary();
                            sleep(4000);
                            if (!gameLibrary.isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 3:
                            //Store-Home
                            if (!navBar.gotoStorePage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 4:
                            //Store-GDP
                            browseGamesPage = navBar.gotoBrowseGamesPage();
                            storeHomeTile = browseGamesPage.getLoadedStoreGameTiles().get(1);
                            storeHomeTile.clickOnTile();
                            gdpHeader = new GDPHeader(driver);
                            gdpHeader.waitForPageToLoad();
                            if (!gdpHeader.verifyGDPHeaderReached()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;

                        /*case 5:
                         //Store-GGG
                         pdpHero = navigateToPdpPage(navBar, driver);
                         pdpHero.scrollToClickAboutGreatGameGuaranteeLink();
                         AboutGreatGameGuaranteePage aboutGreatGameGuaranteePage = new AboutGreatGameGuaranteePage(driver);
                         aboutGreatGameGuaranteePage.waitForPageToLoad();
                         aboutGreatGameGuaranteePage.waitForJQueryAJAXComplete(10);
                         boolean isAboutGreatGameGuaranteePageReached = MiscUtilities.pollingWait(() -> aboutGreatGameGuaranteePage.verifyAboutGreatGameGuaranteeTextVisible());
                         if (!isAboutGreatGameGuaranteePageReached) {
                         listofPagesFailedToLoad.add(page);
                         }
                         break;
                         case 6:
                         //Store-GGGPolicy
                         pdpHero = navigateToPdpPage(navBar, driver);
                         pdpHero.scrollToClickGreatGameGuaranteePolicyLink();
                         GreatGameGuaranteePolicyPage greatGamePolicyPage = new GreatGameGuaranteePolicyPage(driver);
                         greatGamePolicyPage.waitForPageToLoad();
                         greatGamePolicyPage.waitForJQueryAJAXComplete(10);
                         boolean isGreatGamePolicyPageReached = MiscUtilities.pollingWait(() -> greatGamePolicyPage.verifyGreatGameGuaranteePolicyPageText());
                         if (!isGreatGamePolicyPageReached) {
                         listofPagesFailedToLoad.add(page);
                         }
                         break;*/
                        case 5:  //game time

                        case 6://on the house page
                            break;
                        case 7:
                            break;
                        case 8://download origin
                            DownloadOriginPage downloadOriginPage = navBar.gotoDownloadOriginPage();
                            boolean isdownloadOriginPageReached = Waits.pollingWait(() -> downloadOriginPage.verifyDownloadOriginTileVisible());
                            if (!isdownloadOriginPageReached) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 9: // store demos and betas
                            if (!navBar.gotoDemosBetasPage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;

                        case 10: // store deals
                            if (!navBar.gotoDealsPage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;

                        case 11: // store bundle  yet to be added to know the stable bundle page is required here
                            break;

                        case 12: // store browse
                            if (!navBar.gotoBrowseGamesPage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 13: // store addon
                            break;
                        case 14: // store about
                            AboutStorePage aboutPage = navBar.gotoAboutStorePage();
                            boolean isAboutPageReached = Waits.pollingWait(() -> aboutPage.verifyAboutStorePageReached());
                            if (!isAboutPageReached) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;

                        case 15: // store origin access
                            if (!navBar.gotoOriginAccessPage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 16: // store origin accesss FAQ
                            OriginAccessFaqPage originAccessFaqPage = navBar.gotoOriginAccessFaqPage();
                            boolean isOriginAccessFaqPageReached = Waits.pollingWait(() -> originAccessFaqPage.verifyFaqTitleMessage());
                            if (!isOriginAccessFaqPageReached) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 17: // store origin access Trials
                            OriginAccessTrialPage originAccessTrialPage = navBar.gotoOriginAccessTrialPage();
                            //wait for animation to complete
                            sleep(2000);
                            if (!originAccessTrialPage.isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 18: // store origin access vaults
                            if (!navBar.gotoVaultGamesPage().isPageLoaded()) {
                                listofPagesFailedToLoad.add(page);
                            }
                            break;
                        case 19: // settings - yet to be added to know the exact settings page required here
                            break;
                        case 20: // settings page yet to be added to know the exact settings page required here
                            break;

                    }
                } catch (Exception ex) {
                    listofPagesFailedToLoad.add(page);
                }
            }
            int sizeOfPageFails = listofPagesFailedToLoad.size();
            String failedPages;
            if (sizeOfPageFails > 0) {
                failedPages = StringUtils.join(listofPagesFailedToLoad, ",");
                if (failedPages.length() > 0) {
                    logFail(failedPages + " failed to load");
                }
            } else {
                logPass("Successfully navigated through all the pages successfully");
            }
            new MiniProfile(driver).selectSignOut();
        }
    }

    @Test(groups = {"browser", "browser_only"})
    public void testPurchase(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        Battlefield4 battlefield4Entitlement = new Battlefield4();

        final WebDriver driver = startClientObject(context, client);
        ArrayList<UserAccount> userAcc = new ArrayList(10);
        for (int i = 0; i < 10; i++) {
            UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
            userAcc.add(userAccount);
            logFlowPoint("Launch Origin and register as a new user" + userAccount.getUsername()); //1
            logFlowPoint("Load " + battlefield4Entitlement.getName() + " Pdp page"); //2
            logFlowPoint("Purchase the game"); //3
            logFlowPoint("Verify game appeared in the gamelibrary after purchase"); //4
            logFlowPoint("Load " + Battlefield4.BF4_BRONZE_BATTLEPACK_NAME + " Pdp page"); //5
            logFlowPoint("Purchase the game"); //6
            sleep(1000);
        }

        for (int i = 0; i < 10; i++) {
            _log.debug("User acc name" + userAcc.get(i).getUsername());
        }
        for (int i = 0; i < 10; i++) {
            try {
                boolean prevStepFailure = false;
                UserAccount userAccount = userAcc.get(i);
                if (MacroLogin.startLogin(driver, userAccount)) {
                    logPass("Successfully created user and logged in as: " + userAccount.getEmail());
                } else {
                    logFail("Could not create user");
                    prevStepFailure = true;
                }

                //2
//                if (!prevStepFailure && MacroPDP.loadPdpPage(driver, battlefield4Entitlement)) {
//                    logPass("Successfully navigated to PDP page");
//                } else {
//                    logFail("Could not navigate to the Product Description Page");
//                    prevStepFailure = true;
//                }

                //3
                if (!prevStepFailure) {
                    // new PDPHeroActionCTA(driver).clickBuyButton();
                    if (MacroPurchase.completePurchaseAndCloseThankYouPage(driver)) {
                        logPass("Successfully purchased the game");
                    } else {
                        logFail("Failed to purchase the game");
                        prevStepFailure = true;
                    }
                } else {
                    logFail("Failed to purchase game");

                }

                if (!prevStepFailure && MacroGameLibrary.verifyGameInLibrary(driver, battlefield4Entitlement.getName())) {
                    logPass("Successfully added to game library");
                } else {
                    logFail("Game not available in library after purchase");
                    prevStepFailure = true;
                }

                //4
//                boolean loadedPage = MacroPDP.loadPdpPage(driver,
//                        battlefield4Entitlement.BF4_BRONZE_BATTLEPACK_NAME,
//                        battlefield4Entitlement.BF4_BRONZE_BATTLEPACK_OFFER_ID,
//                        battlefield4Entitlement.BF4_BRONZE_BATTLEPACK_PARTIAL_PDP_URL);
//                if (!prevStepFailure && loadedPage) {
//                    logPass("Successfully navigated to PDP add on page");
//                } else {
//                    logFail("Could not navigate to the Product Description Page");
//                    prevStepFailure = true;
//                }

                //5
                if (!prevStepFailure) {
                    // new PDPHeroActionCTA(driver).clickBuyButton();
                    if (MacroPurchase.completePurchaseAndCloseThankYouPage(driver)) {
                        logPass("Successfully purchased the addon game");
                    } else {
                        logFail("Failed to purchase the game");
                    }
                } else {
                    logFail("Failed to purchase addon");
                }

            } catch (Exception ex) {
                logFail("Failure:" + ex.toString());

            }
            new MiniProfile(driver).selectSignOut();

        }
    }

}
