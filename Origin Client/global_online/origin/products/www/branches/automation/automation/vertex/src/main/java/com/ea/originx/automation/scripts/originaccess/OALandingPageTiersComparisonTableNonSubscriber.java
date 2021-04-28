package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies the comparison table under the 'Compare plan' section
 *
 * @author cbalea
 */
public class OALandingPageTiersComparisonTableNonSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3246566)
    @Test(groups = {"origin_access", "full_regression"})
    public void testLandingPageTierComparisonTableNonSubscriber(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        if (isClient) {
            logFlowPoint("Log into Origin with a non-subscriber account"); // 1
        }
        logFlowPoint("Navigate to 'Origin Access' landing page and verify there is a comparison table"); // 2
        logFlowPoint("Verify there is a 'Compare Plan' navigation item listed on the navigation bar"); // 3
        logFlowPoint("Verify clicking on the navigation bar option scrolls up to the compare table"); // 4
        logFlowPoint("Verifiy the comparison table rows mentions differences between 'Basic' and 'Premier'"); // 5
        logFlowPoint("Verify the comparison table has two columns one for 'Basic' and one for 'Premier' Tiers"); // 6
        logFlowPoint("Verify there are two different logos for 'Basic' and 'Premier'"); // 7
        logFlowPoint("Verify the 'Basic' column header displays the monthly and yearly price"); // 8
        logFlowPoint("Verify the 'Premier' column header displays the monthly and yearly price"); // 9
        logFlowPoint("Verify the comparison table has CTAs to puchase 'Basic' and 'Premier' subscription"); // 10
        logFlowPoint("Verify the comparison table can have either light or dark background"); // 11

        // 1
        WebDriver driver = startClientObject(context, client);
        if (isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
            new MainMenu(driver).clickMaximizeButton();
        }

        // 2
        OriginAccessPage originAccessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        originAccessPage.waitForPageToLoad();
        logPassFail(originAccessPage.verifyComparePlanSection(), true);

        // 3
        logPassFail(originAccessPage.verifyComparePlanTabButton(), false);

        // 4
        originAccessPage.clickComparePlanTab();
        logPassFail(originAccessPage.verifyComparePlanSection(), true);

        // 5
        logPassFail(originAccessPage.verifyComparisonTableTierDifferences(), false);

        // 6
        boolean isBasicColumn = originAccessPage.verifyBasicTierColumn();
        boolean isPremierColumn = originAccessPage.verifyPremierTierColumn();
        logPassFail(isBasicColumn && isPremierColumn, false);

        // 7
        boolean isBasicLogo = originAccessPage.verifyBasicTierLogo();
        boolean isPremierLogo = originAccessPage.verifyPremierTierLogo();
        logPassFail(isBasicLogo && isPremierLogo, false);

        // 8
        logPassFail(originAccessPage.verifyBasicTierPrice(), false);

        // 9
        logPassFail(originAccessPage.verifyPremierTierPrice(), false);

        // 10
        boolean isBasicCTA = originAccessPage.verifyBasicTierCTAButton();
        boolean isPremierCTA = originAccessPage.verifyPremierTierCTAButton();
        logPassFail(isBasicCTA && isPremierCTA, false);

        // 11
        logPassFail(originAccessPage.verifyComparisonTableBackgroundColor(), true);

        softAssertAll();
    }
}
