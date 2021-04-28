package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.helpers.EACoreHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.Notification;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.lib.resources.games.MassEffect3;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.utils.AchievementService;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests achievement event listener by granting achievement through REST API
 */
public class OAAchievementNotification extends EAXVxTestTemplate {

    @TestRail(caseId = 1016732)
    @Test(groups = {"client_only", "services_smoke"})
    public void testAchievementNotification(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final String accessToken = userAccount.getAuthToken();
        final String personaId = userAccount.getPersonaId();
        EACoreHelper.extendNotificationExpiryTime(client.getEACore());

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ME3);
        final String entitlementName = entitlement.getName();

        final String achievementName = MassEffect3.ME3_ACHIEVEMENT_DRIVEN_NAME; // name for the ME3 achievement
        final String productIdForAchievement = MassEffect3.ME3_ACHIEVEMENT_PRODUCT_ID; // product id for mass effect 3
        final String achievementID = MassEffect3.ME3_ACHIEVEMENT_DRIVEN_ID; // id for achievement 'Driven'
        final String point_value = "4";

        logFlowPoint("Login to client with newly created account"); //1
        logFlowPoint("Purchase entitlement " + entitlementName); //2
        logFlowPoint("Grant achievement " + achievementName + " for " + entitlementName + " through REST API"); //3
        logFlowPoint("Verify toasty notification appears for achievement: " + achievementName + " for " + entitlementName); //4

        //1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        logPassFail(MacroPurchase.purchaseEntitlement(driver, entitlement), true);

        //3
        sleep(3000); // sever needs some time to update entitlement info
        boolean isAchievementGrantedThroughREST = StringHelper.containsAnyIgnoreCase(AchievementService.grantAchievementRESTAPI(accessToken, personaId, productIdForAchievement, achievementID, point_value), "achievedPercentage");
        logPassFail(isAchievementGrantedThroughREST, true);

        //4
        logPassFail(new Notification(driver).verifyToastyNotificationTitle(achievementName), true);

        softAssertAll();
    }
}