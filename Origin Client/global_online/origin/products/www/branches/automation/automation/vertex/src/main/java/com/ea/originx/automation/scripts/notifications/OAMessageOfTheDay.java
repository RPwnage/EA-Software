package com.ea.originx.automation.scripts.notifications;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.pageobjects.store.MessageOftheDay;
import com.ea.originx.automation.lib.pageobjects.store.StorePage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the basic functionality of the store Message of the Day
 *
 * @author rmcneil
 */
public class OAMessageOfTheDay extends EAXVxTestTemplate {

    @TestRail(caseId = 12047)
    @Test(groups = {"notifications", "full_regression"})
    public void testMessageOfTheDay(ITestContext context) throws Exception {

        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManager.getRandomAccount();


        logFlowPoint("Log into Origin with " + user.getUsername()); //1
        logFlowPoint("Verify MOTD not visible on Discover Page"); //2
        logFlowPoint("Verify MOTD not visible on Game Library Page"); //3
        if (isClient) {
            logFlowPoint("Verify MOTD not visible on App Settings Page"); //4
        }
        logFlowPoint("Verify MOTD visible on Store Page"); //5
        logFlowPoint("Verify only one MOTD visible"); //6
        logFlowPoint("Verify MOTD is inside Store container"); //7
        logFlowPoint("Verify MOTD can be closed"); //8
        logFlowPoint("Navigate to Game Library Page"); //9
        logFlowPoint("Navigate back to Store Page and verify MOTD does not reappear"); //10

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, user)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        NavigationSidebar navBar = new NavigationSidebar(driver);
        DiscoverPage discoverPage = navBar.gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        MessageOftheDay messageOfTheDay = new MessageOftheDay(driver);
        boolean discoverPageReached = discoverPage.verifyDiscoverPageReached();
        boolean motdVisible = messageOfTheDay.verifyMOTDVisible();
        if (discoverPageReached && !motdVisible) {
            logPass("Discover page reached and no MOTD visible.");
        } else {
            logFail("Discover page not reached or Message of the day visible.");
        }

        //3
        GameLibrary gameLibrary = navBar.gotoGameLibrary();
        boolean gameLibraryReached = gameLibrary.verifyGameLibraryPageReached();
        motdVisible = messageOfTheDay.verifyMOTDVisible();
        if (gameLibraryReached && !motdVisible) {
            logPass("Game Library page reached and no MOTD visible.");
        } else {
            logFail("Game Library page not reached or Message of the day visible.");
        }

        //4
        //Settings page isn't available on browser
        if (isClient) {
            AppSettings appSettings = new MainMenu(driver).selectApplicationSettings();
            boolean appSettingsReached = appSettings.verifyAppSettingsReached();
            motdVisible = messageOfTheDay.verifyMOTDVisible();
            if (appSettingsReached && !motdVisible) {
                logPass("App Settings page reached and no MOTD visible.");
            } else {
                logFail("App Settings page not reached or Message of the day visible.");
            }
        }

        //5
        StorePage storePage = navBar.gotoStorePage();
        //Sometimes takeover page does exist
        new TakeOverPage(driver).closeIfOpen();

        boolean storePageReached = storePage.verifyStorePageReached();
        motdVisible = messageOfTheDay.verifyMOTDVisible();
        if (storePageReached && motdVisible) {
            logPass("Store Page reached and Message of the day visible.");
        } else {
            logFail("Store page not reached or Message of the day visible.");
        }

        //6
        if (messageOfTheDay.verifyOnlyOneMOTD()) {
            logPass("Only one Message of the Day visible.");
        } else {
            logFail("More than one Message of the Day visible.");
        }

        //7
        if (messageOfTheDay.verifyMOTDInStoreContainer()) {
            logPass("Message of the Day is inside the store container.");
        } else {
            logFail("Message of the Day is not inside the store container.");
        }

        //8
        messageOfTheDay.closeMOTD();
        if (!messageOfTheDay.verifyMOTDVisible()) {
            logPass("Message of the Day was closed successfully.");
        } else {
            logFail("Message of the Day was not closed successfully.");
        }

        //9
        gameLibrary = navBar.gotoGameLibrary();
        if (gameLibrary.verifyGameLibraryPageReached()) {
            logPass("Game Library Page reached.");
        } else {
            logFail("Game Library Page was not reached.");
        }

        //10
        storePage = navBar.gotoStorePage();
        //Sometimes takeover page does exist
        new TakeOverPage(driver).closeIfOpen();
        storePageReached = storePage.verifyStorePageReached();
        motdVisible = messageOfTheDay.verifyMOTDVisible();
        if (storePageReached && !motdVisible) {
            logPass("Store Page reached and Message of the day remained closed.");
        } else {
            logFail("Store Page reached and Message of the day visible again.");
        }
        softAssertAll();
    }
}
