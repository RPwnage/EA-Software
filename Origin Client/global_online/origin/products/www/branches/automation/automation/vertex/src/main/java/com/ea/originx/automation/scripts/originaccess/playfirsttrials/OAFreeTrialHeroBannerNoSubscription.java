package com.ea.originx.automation.scripts.originaccess.playfirsttrials;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroCommon;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNavigation;
import com.ea.originx.automation.lib.macroaction.MacroStore;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.originaccess.PlayFirstTrialsPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * To verify the hero on the Play First Trials Page
 *
 * @author svaghayenegar
 */
public class OAFreeTrialHeroBannerNoSubscription extends EAXVxTestTemplate {

    @TestRail(caseId = 12098)
    @Test(groups = {"originaccess", "full_regression"})
    public void OAFreeTrialHeroBannerNoSubscription(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManagerHelper.getUnentitledUserAccountWithCountry("Canada");
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        if(isClient){
            logFlowPoint("Log into Origin as a non-subscriber"); // 1
        }

        logFlowPoint("Hover over the Origin Access tab in the left navigation menu, click on the Play First Trials option, and verify page loads"); //2
        logFlowPoint("Verify that the 'Origin Access' logo is displayed correctly"); //3
        logFlowPoint("Verify that there is a title for the hero"); //4
        logFlowPoint("Verify bullet points appear explaining the premier and basic member benefits"); //5
        logFlowPoint("Verify that there is a PC only program text on the hero"); //6
        logFlowPoint("Verify that a background image displays on the hero banner"); //7

        WebDriver driver = startClientObject(context, client);

        // 1
        if (isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        }

        //2
        NavigationSidebar navigationSidebar = new NavigationSidebar(driver);
        navigationSidebar.clickOriginAccessTrialLink();
        PlayFirstTrialsPage playFirstTrialsPage = new PlayFirstTrialsPage(driver);
        logPassFail(playFirstTrialsPage.verifyPlayFirstTrialPageReached(), true);

        //3
        logPassFail(playFirstTrialsPage.verifyHeroOriginAccessLogoIsVisible(), true);

        //4
        logPassFail(playFirstTrialsPage.verifyHeroTitle(), true);

        //5
        logPassFail(playFirstTrialsPage.verifyHeroBulletPoints(), true);

        //6
        logPassFail(playFirstTrialsPage.verifyHeroPlatform(), true);

        //7
        logPassFail(playFirstTrialsPage.verifyHeroBackgroundImageIsDisplayed(), true);

        softAssertAll();
    }
}