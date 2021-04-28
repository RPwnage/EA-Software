package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameSlideout;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.store.PaymentInformationPage;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Check 'Origin access expired' violator text in gamelibrary and OGD
 *
 * @author cdeaconu
 */
public class OARenewOriginAccessMembershipViolator extends EAXVxTestTemplate{
    
    @TestRail(caseId = 10866)
    @Test(groups = {"originaccess", "full_regression"})
    public void testRenewOriginAccessMembershipViolator(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_PREMIUM);
        final String offerId = entitlement.getOfferId();
        Criteria criteria = new Criteria.CriteriaBuilder().tag(AccountTags.SUBSCRIPTION_EXPIRED_OWN_ENT.name()).build();
        criteria.addEntitlement(entitlement.getName());
        final UserAccount userAccount = AccountManager.getInstance().requestWithCriteria(criteria);
        
        logFlowPoint("Log into Origin with an expired subscriber account."); // 1
        logFlowPoint("Navigate to 'Game Library'."); // 2
        logFlowPoint("verify that the text violator 'Your Origin Access membership has expired' is displayed in game tile."); // 3
        logFlowPoint("Verify that there's a red warning icon."); // 4
        logFlowPoint("Navigate to the OGD and verify that the text 'Your Origin Access membership has expired. Renew membership or buy the game to continue playing.' is displayed."); // 5
        logFlowPoint("Verify clicking 'Renew membership' link will trigger membership renewal flow."); // 6
        
        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        
        // 2
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        logPassFail(gameLibrary.verifyGameLibraryPageReached(), true);
        
        // 3
        GameTile gameTile = new GameTile(driver, offerId);
        logPassFail(gameTile.verifyViolatorStatingMembershipIsExpired(), false);
        
        // 4
        logPassFail(gameTile.verifyViolatorIconColor(), false);
        
        // 5
        GameSlideout gameSlideout = new GameTile(driver, offerId).openGameSlideout();
        logPassFail(gameSlideout.verifyOriginAccessExpiredTextVisible(), false);
        
        // 6
        gameSlideout.upgradeEntitlement();
        PaymentInformationPage paymentInformationPage = new PaymentInformationPage(driver);
        paymentInformationPage.waitForPaymentInfoPageToLoad();
        logPassFail(paymentInformationPage.verifyPaymentInformationReached(), true);
        
        softAssertAll();
    }
}