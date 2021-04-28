package com.ea.originx.automation.scripts.feature.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog.Eligibility;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPActionCTA;
import com.ea.originx.automation.lib.pageobjects.gdp.OfferSelectionPage;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests gifting a game to a user who already owns it.
 *
 * NEED UPDATE TO GDP
 *
 * @author jmittertreiner
 */
public class OASendGiftOwned extends EAXVxTestTemplate {

    @TestRail(caseId = 3087366)
    @Test(groups = {"gifting", "gifting_smoke", "int_only", "allfeaturesmoke"})
    public void testSendGiftOwned(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final Battlefield4 entitlement = new Battlefield4();
        final UserAccount userAccount = AccountManagerHelper.getEntitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA, entitlement);
        final UserAccount ownsBattlefieldAccount = AccountManagerHelper.getEntitledUserAccountWithCountry(OriginClientConstants.COUNTRY_CANADA, entitlement);
        final UserAccount unentitledAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        userAccount.cleanFriends();
        ownsBattlefieldAccount.cleanFriends();
        unentitledAccount.cleanFriends();
        UserAccountHelper.addFriends(userAccount, ownsBattlefieldAccount, unentitledAccount);

        final String ownsBattlefieldName = ownsBattlefieldAccount.getUsername();
        final String unentitledName = unentitledAccount.getUsername();
        final String dlcName = Battlefield4.BF4_FINAL_STAND_NAME;

        logFlowPoint("Launch Origin and login."); // 1
        logFlowPoint("Navigate to the GDP of an owned entitlement and click 'Purchase as a Gift' dropdown-menu item."); // 2
        logFlowPoint("Verify " + ownsBattlefieldName + " show as not eligible to gift to."); // 3
        logFlowPoint("Verify " + unentitledName + " show as eligible to gift to."); // 4
        logFlowPoint("Navigate to the GDP of " + dlcName + " and click the buy as gift button."); // 5
        logFlowPoint("Verify " + ownsBattlefieldName + " show as eligible to gift to."); // 6
        logFlowPoint("Verify " + unentitledName + " show as not eligible to gift to."); // 7

        // 1
        final WebDriver driver = startClientObject(context, client, ORIGIN_CLIENT_DEFAULT_PARAM, "/can/en-us");
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
    //   MacroGDP.loadGdpPage(driver, entitlement);
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        OfferSelectionPage offerSelectionPage = new OfferSelectionPage(driver);
        offerSelectionPage.verifyOfferSelectionPageReached();
        offerSelectionPage.clickPrimaryButton(entitlement.getOcdPath());
        final FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        // 3
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(ownsBattlefieldAccount));
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(unentitledAccount));
        Eligibility eligibility = friendsSelectionDialog.getRecipient(ownsBattlefieldAccount).getEligibility();
        logPassFail(eligibility == Eligibility.ALREADY_OWNS || eligibility == Eligibility.INELIGIBLE, false);

        // 4
        eligibility = friendsSelectionDialog.getRecipient(unentitledAccount).getEligibility();
        logPassFail(eligibility == Eligibility.ELIGIBLE || eligibility == Eligibility.ON_WISHLIST, false);
        friendsSelectionDialog.clickCloseCircle();

        // 5
       // MacroGDP.loadGdpPage(driver, dlcName, Battlefield4.BF4_FINAL_STAND_PARTIAL_PDP_URL);
        new GDPActionCTA(driver).selectDropdownPurchaseAsGift();
        logPassFail(friendsSelectionDialog.waitIsVisible(), true);

        // 6
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(ownsBattlefieldAccount));
        Waits.pollingWait(() -> friendsSelectionDialog.isRecipientPresent(unentitledAccount));
        boolean isEligible = Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(ownsBattlefieldAccount).getEligibility() == Eligibility.ELIGIBLE);
        logPassFail(isEligible, false);

        // 7
        boolean isIneligible = Waits.pollingWait(() -> friendsSelectionDialog.getRecipient(unentitledAccount).getEligibility() == Eligibility.INELIGIBLE);
        logPassFail(isIneligible, false);

        softAssertAll();
    }
}