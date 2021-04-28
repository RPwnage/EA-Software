package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemCompleteDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemConfirmDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.RedeemDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test for Redeem dialog. Used by the following parameterized test cases:
 * - OARedeemDipSmall 
 * - OARedeemDipLarge 
 * - OARedeemNonDipSmall 
 * - OARedeemNonDipLarge 
 * - OARedeemDipSmallUnderage 
 * - OARedeemDipLargeUnderage 
 * - OARedeemNonDipSmallUnderage 
 * - OARedeemNonDipLargeUnderage 
 * 
 * @author palui
 */
public class OARedeemDialog extends EAXVxTestTemplate {

    /**
     * Test method supporting parameterized test cases for the Redeem dialog
     *
     * @param context       context used
     * @param entitlementId the entitlement to use
     * @param isUnderage    if the test is to be conducted with an underage account
     * 
     * @throws Exception
     */
    public void testRedeemDialog(ITestContext context, EntitlementId entitlementId, boolean isUnderage) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // Set up entitlement according to parameters
        EntitlementInfo entitlementInfo = EntitlementInfoHelper.getEntitlementInfoForId(entitlementId);
        String entitlementName = entitlementInfo.getName();
        String entitlementOfferId = entitlementInfo.getOfferId();
        String entitlementProductCode = entitlementInfo.getProductCode();
        UserAccount userAccount;
        
        if (isUnderage) {
            userAccount = AccountManagerHelper.createNewThrowAwayAccount(6); // Underage account 
        } else {
            userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        }
        
        final WebDriver driver = startClientObject(context, client);
        
        logFlowPoint("Launch Origin and login as a newly registered user"); // 1
        logFlowPoint("Select 'Redeem Product Code' from the 'Origin' menu"); // 2
        logFlowPoint("Enter product code for the entitlement, click 'Next' button, and verify that the confirm dialog appears"); // 3
        logFlowPoint("Click 'Next' to open the 'Redeem Complete' dialog"); // 4
        logFlowPoint("Go to 'Game Library' page and verify that the entitlement is visible"); // 5

        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);
         
        // 2                 
        new MainMenu(driver).selectRedeemProductCode(isUnderage);                
        RedeemDialog redeemDialog = new RedeemDialog(driver);
        redeemDialog.waitAndSwitchToRedeemProductDialog();
        redeemDialog.waitIsVisible();
        logPassFail(redeemDialog.isOpen(), true);
        
        // 3
        redeemDialog.enterProductCode(entitlementProductCode);
        redeemDialog.clickNextButton();
        RedeemConfirmDialog confirmDialog = new RedeemConfirmDialog(driver);
        confirmDialog.waitAndSwitchToRedeemConfirmDialog();
        confirmDialog.waitForVisible();
        boolean isConfirmDialogTitleCorrect = confirmDialog.verifyRedeemConfirmDialogTitle();
        boolean isConfirmDialogProductCorrect = confirmDialog.verifyRedeemConfirmDialogProductTitle(entitlementName);
        logPassFail(isConfirmDialogTitleCorrect && isConfirmDialogProductCorrect, true);

        // 4
        confirmDialog.clickConfirmButton();
        RedeemCompleteDialog completeDialog = new RedeemCompleteDialog(driver);
        completeDialog.waitAndSwitchToRedeemCompleteDialog();
        completeDialog.waitForVisible();
        boolean isCompleteDialogTitleCorrect = completeDialog.verifyRedeemCompleteDialogActivationTitle();
        boolean isCompleteDialogProductCorrect = completeDialog.verifyRedeemCompleteDialogProductTitle(entitlementName);
        logPassFail(isCompleteDialogTitleCorrect && isCompleteDialogProductCorrect, true);
        
        // 5
        completeDialog.clickCloseButton();
        boolean isRedeemCompleteDialogClosed = !completeDialog.isRedeemCompleteDialogOpen();
        new NavigationSidebar(driver).gotoGameLibrary();
        // forcing a refresh now because 'Game library' isn't automatically refresh 
        new MainMenu(driver).selectRefresh(isUnderage);
        GameTile gameTile = new GameTile(driver, entitlementOfferId);
        boolean isTitleCorrect = gameTile.verifyTitle(entitlementName);
        logPassFail(isRedeemCompleteDialogClosed && isTitleCorrect, true);
        
        softAssertAll();
    }
}