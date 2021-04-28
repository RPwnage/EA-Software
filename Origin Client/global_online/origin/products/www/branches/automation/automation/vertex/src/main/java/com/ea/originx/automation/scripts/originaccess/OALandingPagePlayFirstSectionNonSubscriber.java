package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.resources.AccountTags;
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
 * Tests contents of 'Play First' section within 'Origin Access' landing page
 *
 * @author cbalea
 */
public class OALandingPagePlayFirstSectionNonSubscriber extends EAXVxTestTemplate {

    @TestRail(caseId = 3286651)
    @Test(groups = {"origin_access", "full_regression"})

    public void testLandingPagePlayFirstSectionNonSubscriber(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.TEST_PDP_NON_SUBSCRIBER);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        if (isClient) {
            logFlowPoint("Log into Origin with a non-subscriber account"); // 1
        }
        logFlowPoint("Navigate to 'Origin Access' landing page"); // 2
        logFlowPoint("Scroll to 'Play First' section and verify the section has a title"); // 3
        logFlowPoint("Verify there are two bullet points explaining how 'Play First' works"); // 4
        logFlowPoint("Verify that there is a background image"); // 5
        logFlowPoint("Verify that there is a foreground image"); // 6
        logFlowPoint("Verify that there is a game logo displayed"); // 7

        // 1
        WebDriver driver = startClientObject(context, client);
        if (isClient) {
            logPassFail(MacroLogin.startLogin(driver, userAccount), true);
            new MainMenu(driver).clickMaximizeButton();
        }

        // 2
        OriginAccessPage originAccessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        logPassFail(originAccessPage.verifyPageReached(), true);

        // 3
        logPassFail(originAccessPage.verifyPlayFirstSectionTitle(), false);

        // 4
        logPassFail(originAccessPage.verifyPlayFirstBulletPoints(2), false);

        // 5
        logPassFail(originAccessPage.verifyPlayFirstBackgroundImage(), false);

        // 6
        logPassFail(originAccessPage.verifyPlayFirstForegroundImage(), false);

        // 7
        logPassFail(originAccessPage.verifyPlayFirstGameLogo(), false);

        softAssertAll();
    }
}