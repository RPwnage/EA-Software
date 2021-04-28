package com.ea.originx.automation.scripts.feature.redeem;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemConfirmDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemDialog;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the case where the user is trying to redeem an already owned
 * entitlement
 *
 * @author cbalea
 */
public class OARedeemAlreadyOwned extends EAXVxTestTemplate {

    @Test(groups = {"redeem", "client_only", "feature_smoke"})
    public void testRedeemAlreadyOwned(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String entitlementProductCode = entitlement.getProductCode();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String entitlementName = entitlement.getName();

        logFlowPoint("Log into Origin with an account which already owns " + entitlementName); // 1
        logFlowPoint("Select 'Redeem Product Code' from the 'Origin' menu"); // 2
        logFlowPoint("Enter redemption code for 'DiP Small', click next and verify 'Redeem Confirmed' dialog appears"); // 3
        logFlowPoint("Click confirm and verify that a message appears stating that the code has already been used"); // 4

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new MainMenu(driver).selectRedeemProductCode();
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.waitAndSwitchToRedeemProductDialog();
        logPassFail(redeemDialog.isOpen(), true);

        // 3
        redeemDialog.enterProductCode(entitlementProductCode);
        redeemDialog.clickNextButton();
        RedeemConfirmDialog redeemConfirmDialog = new RedeemConfirmDialog(driver);
        redeemConfirmDialog.waitAndSwitchToRedeemConfirmDialog();
        logPassFail(redeemConfirmDialog.verifyRedeemConfirmDialogProductTitle(entitlementName), false);

        // 4
        redeemConfirmDialog.clickConfirmButton();
        logPassFail(redeemDialog.verifyCodeAlreadyUsedWarning(), true);

        softAssertAll();
    }
}
