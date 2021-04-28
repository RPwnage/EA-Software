package com.ea.originx.automation.scripts.gifting;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroGifting;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.NotificationCenter;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.OpenGiftPage;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.settings.InstallSaveSettings;
import com.ea.originx.automation.lib.pageobjects.settings.SettingsNavBar;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests launching a received gift.
 *
 * @author glivingstone
 */
public class OALaunchGift extends EAXVxTestTemplate {

    @Test(groups = {"gifting", "client_only", "full_regression", "int_only"})
    public void testLaunchGift(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount receiver = AccountManagerHelper.registerNewThrowAwayAccountThroughREST(OriginClientConstants.COUNTRY_CANADA);
        final UserAccount sender = AccountManagerHelper.getTaggedUserAccountWithCountry(AccountTags.NO_ENTITLEMENTS, OriginClientConstants.COUNTRY_CANADA);
        String receiverName = receiver.getUsername();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.UNRAVEL);
        final String entitlementName = entitlement.getName();
        final String entitlementOfferID = entitlement.getOfferId();

        receiver.cleanFriends();
        sender.addFriend(receiver);

        logFlowPoint("Set up an Account with cloud save disabled to receive " + entitlementName + " as a Gift"); // 1
        logFlowPoint("Log into a User Who is Freinds with the Receiver"); // 2
        logFlowPoint("Send " + entitlementName + " to the Gift Receiver"); // 3
        logFlowPoint("Log Out of the Gift Sender and Log into the Gift Receiver"); // 4
        logFlowPoint("Navigate to My Home and Open the Gift"); // 5
        logFlowPoint("Click the 'Add to the Library and Download Later'"); // 6
        logFlowPoint("Navigate to the Game Library and Download and Install " + entitlementName); // 7
        logFlowPoint("Launch " + entitlementName); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        boolean receiverSetupOK = MacroLogin.startLogin(driver, receiver);
        new MainMenu(driver).selectApplicationSettings();
        new SettingsNavBar(driver).clickInstallSaveLink();
        InstallSaveSettings isSettings = new InstallSaveSettings(driver);
        boolean installSaveOK = Waits.pollingWait(isSettings::verifyInstallSaveSettingsReached);
        isSettings.toggleSaves();
        new MainMenu(driver).selectLogOut();
        if (receiverSetupOK && installSaveOK) {
            logPass(String.format("Successfully set up the gift receiver %s with cloud save disabled", receiverName));
        } else {
            logFailExit(String.format("Could not set up the gift receiver %s with cloud save disabled", receiverName));
        }

        // 2
        if (MacroLogin.startLogin(driver, sender)) {
            logPass("Successfully logged into the gift sender");
        } else {
            logFailExit("Could not log into the gift sender");
        }

        // 3
        if (MacroGifting.purchaseGiftForFriend(driver, entitlement, receiver.getUsername(), "Gift Launch")) {
            logPass("Successfully sent a gift to the receiver.");
        } else {
            logFailExit("Could not send a gift to the receiver.");
        }

        // 4
        new MainMenu(driver).selectLogOut();
        if (MacroLogin.startLogin(driver, receiver)) {
            logPass("Selecting a friend deselects the previously selected friend");
        } else {
            logFailExit("Selecting a friend doesn't deselect the previously selected friend");
        }

        // 5
        NotificationCenter notificationCenter = new NotificationCenter(driver);
        new TakeOverPage(driver).closeIfOpen();
        notificationCenter.clickYouGotGiftNotification();
        OpenGiftPage openGiftPage = new OpenGiftPage(driver);
        if (openGiftPage.waitForVisible()) {
            logPass("Successfully opened the gift.");
        } else {
            logFailExit("Could not open the gift.");
        }

        // 6
        openGiftPage.clickDownloadLater();
        if (!openGiftPage.waitForVisible()) {
            logPass("Selected download later for the gift.");
        } else {
            logFailExit("Could not select download later for the gift.");
        }

        // 7
        new NavigationSidebar(driver).gotoGameLibrary();
        if (MacroGameLibrary.downloadFullEntitlement(driver, entitlementOfferID)) {
            logPass("Successfully downloaded and installed " + entitlementName + ".");
        } else {
            logFailExit("Could not download and install " + entitlementName + ".");
        }

        // 8
        new GameTile(driver, entitlementOfferID).play();
        entitlement.addActivationFile(client);
        if (entitlement.isLaunched(client)) {
            logPass("Successfully launched " + entitlementName + ".");
        } else {
            logFailExit("Could not launch " + entitlementName + ".");
        }

        softAssertAll();
    }
}
