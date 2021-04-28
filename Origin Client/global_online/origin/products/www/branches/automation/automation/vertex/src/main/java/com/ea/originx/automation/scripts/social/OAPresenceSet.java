package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests various Presence options of an user
 *
 * @author mkalaivanan
 */
public class OAPresenceSet extends EAXVxTestTemplate {

    @TestRail(caseId = 13193)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testPresenceSet(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch and login to Origin with " + userAccount.getUsername()); //1
        logFlowPoint("Set Status to Away"); //2
        logFlowPoint("Verify Presence Dot is set to Away"); //3
        logFlowPoint("Set Status to Invisible"); //4
        logFlowPoint("Verify Presence Dot is Invisible"); //5
        logFlowPoint("Set Status to Online"); //6
        logFlowPoint("Verify Presence Dot is set to Online"); //7
        logFlowPoint("Click on the Presence Menu and Verify it is Open"); //8
        logFlowPoint("Click Somewhere Else and Verify the Presence Menu Closes"); //9

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user");
        } else {
            logFailExit("Could not log into Origin with the user");

        }

        //2
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        socialHub.setUserStatusAway();
        if (socialHub.verifyUserAway()) {
            logPass("Set status to Away successfully");
        } else {
            logFailExit("Could not set status to Away");
        }

        //3
        if (socialHub.verifyPresenceDotAway()) {
            logPass("Presence dot matches the Away status");
        } else {
            logFail("Presence dot does not match the Away status");
        }

        //4
        socialHub.setUserStatusInvisible();
        if (socialHub.verifyUserInvisible()) {
            logPass("Set status to Invisible successfully");
        } else {
            logFailExit("Could not set status to Invisible");
        }

        //5
        if (socialHub.verifyPresenceDotInvisible()) {
            logPass("Presence dot matches the Invisible status");
        } else {
            logFail("Presence dot does not match the Invisible status");
        }

        //6
        socialHub.setUserStatusOnline();
        if (socialHub.verifyUserOnline()) {
            logPass("Set status to Online successfully");
        } else {
            logFailExit("Presence dot does not match the Online status");
        }

        //7
        if (socialHub.verifyPresenceDotOnline()) {
            logPass("Presence dot matches the Online status");
        } else {
            logFail("Presence dot does not match the Online status");
        }

        //8
        socialHub.clickUserPresenceDropDown();
        if (socialHub.verifyUserPresenceDropDownVisible()) {
            logPass("Opened status selection menu");
        } else {
            logFailExit("Could not open status selection menu");
        }

        //9
        socialHub.clickOnSocialHubHeader();
        boolean userPresenceDropDownMenuNotVisible = !socialHub.verifyUserPresenceDropDownVisible();
        if (userPresenceDropDownMenuNotVisible) {
            logPass("Closed status selection menu by clicking elsewhere");
        } else {
            logFailExit("Could not close the status selection menu by clicking elsewhere");
        }
        softAssertAll();
    }
}
