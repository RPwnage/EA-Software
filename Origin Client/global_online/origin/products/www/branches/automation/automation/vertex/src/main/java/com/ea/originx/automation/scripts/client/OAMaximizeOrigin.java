package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.Dimension;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that you can maximize and restore the client window
 *
 * @author jdickens 
 */
public class OAMaximizeOrigin extends EAXVxTestTemplate {
    @TestRail(caseId = 10017)
    @Test(groups = {"client", "full_regression", "client_only"})
    public void testMaximizeOrigin(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        String userAccountName = userAccount.getUsername();

        logFlowPoint("Log into the Origin client with a random user account"); // 1
        logFlowPoint("Click the 'Maximize' button and verify that the client window maximizes"); // 2
        logFlowPoint("Click the 'Restore' button and verify that the window restores to its prior size"); // 3

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccountName);
        } else {
            logFailExit("Could not log into Origin with the user " + userAccountName);
        }

        // 2
        MainMenu mainMenu = new MainMenu(driver);
        Dimension dimensionBeforeMaximize = EAXVxSiteTemplate.getOriginClientWindowSize(driver);
        mainMenu.clickMaximizeButton();
        boolean isClientWindowMaximizeButtonHidden = !mainMenu.verifyMaximizeButtonVisible();
        Dimension dimensionAfterMaximize = EAXVxSiteTemplate.getOriginClientWindowSize(driver);
        boolean isWindowMaximized = dimensionBeforeMaximize.getWidth() < dimensionAfterMaximize.getWidth() || dimensionBeforeMaximize.getHeight() < dimensionAfterMaximize.getHeight();
        if (isClientWindowMaximizeButtonHidden && isWindowMaximized) {
            logPass("Successfully maximized the client");
        } else {
            logFail("Failed to maximize the client");
        }

        // 3
        mainMenu.clickRestoreButton();
        boolean isClientWindowRestoreButtonHidden = !mainMenu.verifyRestoreButtonVisible();
        boolean isClientWindowSizeRestored = dimensionBeforeMaximize.equals(EAXVxSiteTemplate.getOriginClientWindowSize(driver));
        if (isClientWindowRestoreButtonHidden && isClientWindowSizeRestored) {
            logPass("Successfully restored the client");
        } else {
            logFail("Failed to restore the client");
        }

        softAssertAll();
    }
}