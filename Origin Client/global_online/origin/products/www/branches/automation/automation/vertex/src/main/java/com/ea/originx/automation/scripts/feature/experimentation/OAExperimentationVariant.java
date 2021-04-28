package com.ea.originx.automation.scripts.feature.experimentation;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test experimentation variants for automation
 *
 * @author RCHOI
 */
public class OAExperimentationVariant extends EAXVxTestTemplate {

    @TestRail(caseId = 3068746)
    @Test(groups = {"experimentation", "browser_only", "experimentation_smoke", "allfeaturesmoke"})
    public void testNavigateSidebar(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        final String AUTOMATION_EXPERIMENT_CONTROL = "control";
        final String AUTOMATION_EXPERIMENT_VARIANT_A = "variantA";
        final String AUTOMATION_EXPERIMENT_VARIANT_B = "variantB";

        logFlowPoint("Log into Origin"); //1
        logFlowPoint("Navigate to game library without override and verify text and image for automation experimentation variant control exist"); //2
        logFlowPoint("Navigate to game library with override from home page and verify text and image for automation experimentation variant A exist"); //3
        logFlowPoint("Navigate to game library with override from home page and verify text and image for automation experimentation variant B exist"); //4
        logFlowPoint("Navigate to game library with override from home page and verify text and image for automation experimentation variant control exist"); //5

        //1
        boolean isClient = ContextHelper.isOriginClientTesing(context);
        final WebDriver driver;
        if (isClient) {
            EACoreHelper.setAutomationExperimentSetting(client.getEACore());
            driver = startClientObject(context, client);
        } else {
            // adding extra parameter for experimentation variants
            driver = startClientObject(context, client, null, null, "&config=https://config.x.origin.com/automation");
        }

        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the username." + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin");
        }

        //2
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.gotoGameLibrary();
        GameLibrary gameLibrary = new GameLibrary(driver);
        if (gameLibrary.verifyAutomationExperiment(AUTOMATION_EXPERIMENT_CONTROL)) {
            logPass("Navigated to game library and verified text and image for automation experimentation variant control exist");
        } else {
            logFailExit("Unable to verify text and image for automation experimentation variant control");
        }

        //3
        navigationSidebar.gotoDiscoverPage();
        sendJavascriptCommand(driver, "Experiment.setVariantOverride(\"automation-experiment\",\"variantA\")");
        navigationSidebar.gotoGameLibrary();
        if (gameLibrary.verifyAutomationExperiment(AUTOMATION_EXPERIMENT_VARIANT_A)) {
            logPass("Navigated to game library with override from home page and verified text and image for automation experimentation variant A exist");
        } else {
            logFailExit("Unable to verify text and image for automation experimentation variant A");
        }

        //4
        navigationSidebar.gotoDiscoverPage();
        sendJavascriptCommand(driver, "Experiment.setVariantOverride(\"automation-experiment\",\"variantB\")");
        navigationSidebar.gotoGameLibrary();
        if (gameLibrary.verifyAutomationExperiment(AUTOMATION_EXPERIMENT_VARIANT_B)) {
            logPass("Navigated to game library with override from home page and verified text and image for automation experimentation variant B exist");
        } else {
            logFailExit("Unable to verify text and image for automation experimentation variant B");
        }

        //5
        navigationSidebar.gotoDiscoverPage();
        sendJavascriptCommand(driver, "Experiment.setVariantOverride(\"automation-experiment\",\"control\")");
        navigationSidebar.gotoGameLibrary();
        if (gameLibrary.verifyAutomationExperiment(AUTOMATION_EXPERIMENT_CONTROL)) {
            logPass("Navigated to game library with override from home page and verified text and image for automation experimentation variant control exist");
        } else {
            logFailExit("Unable to verify text and image for automation experimentation variant control");
        }

        softAssertAll();
    }
}
