package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.originx.automation.lib.macroaction.*;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.GiftingBannedDialog;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileWishlistTab;
import com.ea.originx.automation.lib.pageobjects.profile.WishlistTile;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to ensure a banned user is unable to gift any giftable entitlement to
 * another user
 *
 * NEEDS UPDATE TO GDP
 *
 * @author cvanichsarn
 */

public class OAGiftBannedUser extends EAXVxTestTemplate {

    @TestRail(caseId = 160955)
    @Test(groups = {"gifting", "full_regression"})
    public void testGiftBannedUser(ITestContext context) throws Exception {

        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount bannedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BANNED);
        final UserAccount friendAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.BF4_ON_WISHLIST);
        final EntitlementInfo friendWishlistItem = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);
        String username = bannedAccount.getUsername();
        EntitlementInfo ownedEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.ME3);
        EntitlementInfo unownedEntitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        //clean friends
        bannedAccount.cleanFriends();
        friendAccount.cleanFriends();
        bannedAccount.addFriend(friendAccount);

        String ownedEntitlementName = ownedEntitlement.getName();
        String unownedEntitlementName = unownedEntitlement.getName();

        logFlowPoint("Log into Origin with a banned account"); //1
        logFlowPoint("Navigate to the PDP of an owned offer and click on the 'Purchase as a gift' CTA, and verify dialog shows up and is accurate."); //2
        logFlowPoint("Navigate to the PDP of an unowned offer and click on 'Purchase as a gift', and verify dialog shows up and is accurate."); //3
        logFlowPoint("Navigate to the wishlist of a friend account who has an offer in their wishlist, click on 'Purchase as a gift', and verify dialog shows up and is accurate"); //4
        logFlowPoint("Click on 'How To Address Your Banned or Suspended Account' in the dialog to verify it is accurate and opens a new browser window"); //5
        logFlowPoint("Verify the email given to the user for contact purposes"); //6

        WebDriver driver = startClientObject(context, client);
        // The friend needs to have their privacy setting set to friends of everyone.  Therefore, we force it before the testing begins
        if (isClient) {
            final WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, friendAccount, PrivacySettingsPage.PrivacySetting.EVERYONE);
            settingsDriver.close();
        } else {
            String originUrl = driver.getCurrentUrl();
            MacroAccount.setPrivacyThroughBrowserLogin(driver, friendAccount, PrivacySettingsPage.PrivacySetting.EVERYONE);
            driver.get(originUrl);
        }

        //1
        if (MacroLogin.startLogin(driver, bannedAccount)) {
            logPass("Successfully logged in with the banned account");
        } else {
            logFailExit("Failed to log in with the banned account");
        }

        //2
//        PDPHeroActionCTA pdpHeroCTA = new PDPHeroActionCTA(driver);
//        pdpHeroCTA.verifyBuyAsGiftButtonVisible();
//        pdpHeroCTA.clickBuyAsGiftButton();
        GiftingBannedDialog giftingBannedDialog = new GiftingBannedDialog(driver);
        giftingBannedDialog.waitIsVisible();
        if (giftingBannedDialog.verifyDialogIsAccurate()) {
            logPass("Verified the dialog box that appears informs the user that they are unable to gift an owned offer while banned");
        } else {
            logFailExit("Failed to verify that the dialog box that appears informs the user that they are unable to gift an owned offer while banned");
        }

        //3
        giftingBannedDialog.clickCloseCircle();
        //  pdpHeroCTA.selectBuyDropdownPurchaseAsGift();
        giftingBannedDialog.waitIsVisible();
        if (giftingBannedDialog.verifyDialogIsAccurate()) {
            logPass("Verified the dialog box that appears informs the user that they are unable to gift an unowned offer while banned");
        } else {
            logFailExit("Failed to verify that the dialog box that appears informs the user that they are unable to gift an unowned offer while banned");
        }

        //4
        giftingBannedDialog.clickCloseCircle();
        MacroProfile.navigateToFriendProfile(driver, friendAccount.getUsername());
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openWishlistTab();
        ProfileWishlistTab profileWishlistTab = new ProfileWishlistTab(driver);
        WishlistTile wishlistTile = profileWishlistTab.getWishlistTile(friendWishlistItem.getOfferId());
        wishlistTile.clickPurchaseAsGiftButton();
        giftingBannedDialog.waitIsVisible();
        if (giftingBannedDialog.verifyDialogIsAccurate()) {
            logPass("Verified the dialog box that appears informs the user that they are unable to gift a friend's wishlist item");
        } else {
            logFailExit("Failed to verify that the dialog box that appears informs the user that they are unable to gift a friend's wishlist item");
        }

        //5
        giftingBannedDialog.clickHelpLink();
        boolean verifyHelpLink;
        // Unable to verify browser open when testing in the client, therefore we just check the link's accuracy
        if (isClient) {
            verifyHelpLink = Waits.pollingWaitEx(()->TestScriptHelper.verifyBrowserOpen(client));
        } else {
            verifyHelpLink = giftingBannedDialog.verifyHelpLinkUrl();
        }
        if (verifyHelpLink) {
            logPass("Verified an external browser window was opened to the correct URL");
        } else {
            logFailExit("Unable to verify that an external browser window was opened or that it had the correct URL");
        }

        //6
        if (giftingBannedDialog.verifyHelpEmailLink()) {
            logPass("Verified the Help Email link is accurate");
        } else {
            logFailExit("Unable to verify that the Help Email link is accurate");
        }

        softAssertAll();
    }
}
