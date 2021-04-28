package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.DateHelper;
import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPHeroActionDescriptors;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the Info Block Play First Trial Date
 * 
 * @author mdobre
 */
public class OAInfoBlockPlayFirstTrialPreloadDate extends EAXVxTestTemplate {

    @TestRail(caseId = 3064036)
    @Test(groups = {"gdp", "full_regression"})
    public void testInfoBlockPlayFirstTrialAvailable(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.SUBSCRIPTION_ACTIVE);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.GDP_PRE_RELEASE_TEST_OFFER);

        logFlowPoint("Log into Origin with an existing account."); // 1
        logFlowPoint("Navigate to GDP of a game that has an upcoming Play First Trial pre-load date."); // 2
        logFlowPoint("Verify an info icon is showing."); //3
        logFlowPoint("Verify 'Release Date: 'date text'' text is showing on the right of the info icon."); //4

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), true);

        //3
        GDPHeroActionDescriptors gdpHeroActionDescriptors = new GDPHeroActionDescriptors(driver);
        logPassFail(gdpHeroActionDescriptors.verifyFirstInfoBlockIconVisible(), true);

        //4
        String firstInfoBlockDate = gdpHeroActionDescriptors.getFirstInfoBlockDate();
        boolean isFirstInfoBlockTextInTheRight = gdpHeroActionDescriptors.verifyFirstInfoBlockTextIsRightOfIcon();
        boolean isDateMatchingFormat = DateHelper.verifyDateMatchesExpectedFormat(firstInfoBlockDate, "MMMM DD, YYYY");
        logPassFail(isFirstInfoBlockTextInTheRight && isDateMatchingFormat, true);

        softAssertAll();
    }
}