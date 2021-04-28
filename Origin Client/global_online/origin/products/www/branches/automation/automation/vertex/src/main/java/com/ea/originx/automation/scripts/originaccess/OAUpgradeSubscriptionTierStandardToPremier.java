package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroOriginAccess;
import com.ea.originx.automation.lib.macroaction.MacroPurchase;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.SelectOriginAccessPlanDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessNUXFullPage;
import com.ea.originx.automation.lib.pageobjects.originaccess.OriginAccessPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.store.ReviewOrderPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Tests the flow of upgrading from a basic subscription to premier
 *
 * @author cbalea
 */
public class OAUpgradeSubscriptionTierStandardToPremier extends EAXVxTestTemplate {

    public void testUpgradeSubscriptionTierStandardToPremier(ITestContext context, boolean yearlySubscription) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        String initialSubscriptionPriceWithVat;
        String premierSubscriptionPriceWithVat;
        String subscriptionReccurence;

        if (yearlySubscription) {
            initialSubscriptionPriceWithVat = OriginClientData.ORIGIN_ACCESS_YEARLY_PLAN_PRICE;
            premierSubscriptionPriceWithVat = OriginClientData.ORIGIN_ACCESS_PREMIER_YEARLY_PLAN_PRICE_WITH_VAT;
            subscriptionReccurence = "Annual";
        } else {
            initialSubscriptionPriceWithVat = OriginClientData.ORIGIN_ACCESS_MONTHLY_PLAN_PRICE;
            premierSubscriptionPriceWithVat = OriginClientData.ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN_PRICE_WITH_VAT;
            subscriptionReccurence = "Monthly";
        }

        logFlowPoint("Launch and log into 'Origin' with a user with 'Basic' " + subscriptionReccurence + " subscription"); // 1
        logFlowPoint("Go to 'Origin Access' landing page"); // 2
        logFlowPoint("Click on 'Join Premier Today' CTA nad verify a plan selection modal with the 'Premier' plans shows up"); // 3
        logFlowPoint("Select the " + subscriptionReccurence + " plan and verify 'Review your order' modal shows up"); // 4
        logFlowPoint("Verify the 'Premier' membership offer is displayed"); // 5
        logFlowPoint("Verify there is a previous membership credit calculated based on the current membership used"); // 6
        logFlowPoint("Verify the new billing amount is mentioned"); // 7
        logFlowPoint("Click on 'Start Membership' CTA and verify NUX is displayed"); // 8
        logFlowPoint("Close NUX and verify the upgrade was successful by viewing the mini profile badge"); // 9

        // 1
        final WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
        if (yearlySubscription) {
            MacroOriginAccess.purchaseOriginAccess(driver);
        } else {
            MacroOriginAccess.purchaseMonthlySubscription(driver);
        }

        // 2
        OriginAccessPage originAccessPage = new NavigationSidebar(driver).gotoOriginAccessPage();
        logPassFail(originAccessPage.verifyPageReached(), true);

        // 3
        originAccessPage.clickJoinPremierSubscriberCta();
        SelectOriginAccessPlanDialog originAccessPlanDialog = new SelectOriginAccessPlanDialog(driver);
        logPassFail(originAccessPlanDialog.waitIsVisible(), true);

        // 4
        if (yearlySubscription) {
            originAccessPlanDialog.clickNext();
        } else {
            originAccessPlanDialog.selectPlan(SelectOriginAccessPlanDialog.ORIGIN_ACCESS_PLAN.MONTHLY_PLAN);
            originAccessPlanDialog.clickNext();
        }
        MacroPurchase.handlePaymentInfoPage(driver);
        ReviewOrderPage reviewOrderPage = new ReviewOrderPage(driver);
        logPassFail(reviewOrderPage.verifyReviewOrderPageReached(), true);

        // 5
        logPassFail(reviewOrderPage.verifySubscriptionPlan("Premier", subscriptionReccurence), true);

        // 6
        logPassFail(reviewOrderPage.verifySubscriptionRefundPriceIsCorrect(initialSubscriptionPriceWithVat), true);

        // 7
        String premierAnnualPrice = StringHelper.formatDoubleToPriceAsString(reviewOrderPage.getTotalCost());
        logPassFail(premierAnnualPrice.equals(premierSubscriptionPriceWithVat), true);

        // 8
        reviewOrderPage.clickPayNow();
        OriginAccessNUXFullPage originAccessNUXFullPage = new OriginAccessNUXFullPage(driver);
        logPassFail(originAccessNUXFullPage.verifyPageReached(), true);

        // 9
        originAccessNUXFullPage.clickSkipNuxGoToVaultLinkPremierSubscriber();
        logPassFail(new MiniProfile(driver).verifyOriginPremierBadgeVisible(), true);

        softAssertAll();
    }
}