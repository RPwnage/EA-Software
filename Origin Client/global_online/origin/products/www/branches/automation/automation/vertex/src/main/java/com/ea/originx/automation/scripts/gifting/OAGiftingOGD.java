package com.ea.originx.automation.scripts.gifting;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadEULASummaryDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadInfoDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.DownloadLanguageSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.FriendsSelectionDialog;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.*;
import com.ea.originx.automation.lib.resources.games.Battlefield4;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Gifting at the 'Extra Content' section of 'Game Slideout' ('Origin Game
 * Details') page by purchasing a base game, an expansion, and an add-on.
 *
 * @author palui
 */
public class OAGiftingOGD extends EAXVxTestTemplate {

    @TestRail(caseId = 11782)
    @Test(groups = {"gifting", "full_regression"})
    public void testGiftingOGD(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final Battlefield4 entitlement = new Battlefield4();
        final String entitlementName = entitlement.getName();
        final String offerId = entitlement.getOfferId();

        final String expansionName = Battlefield4.BF4_FINAL_STAND_NAME;
        final String expansionOfferId = Battlefield4.BF4_FINAL_STAND_OFFER_ID;
        final String addonName = Battlefield4.BF4_AIR_VEHICLE_SHORTCUT_KIT_NAME;
        final String addonOfferId = Battlefield4.BF4_AIR_VEHICLE_SHORTCUT_KIT_OFFER_ID;

        UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlementName, expansionName, addonName);
        String username = userAccount.getUsername();

        boolean isClient = ContextHelper.isOriginClientTesing(context);
        String expectedButtonLabel = isClient ? "Download" : "Install Origin & Play";

        logFlowPoint(String.format("Launch Origin and login as user %s entitled to base game %s, expansion %s, and add-on %s",
                username, entitlementName, expansionName, addonName)); //1
        logFlowPoint(String.format("Navigate to Game Library and locate %s's game tile", entitlementName)); //2
        logFlowPoint(String.format("Open Game Slideout for %s and navigate to the 'Extra Content' section", entitlementName)); //3
        logFlowPoint(String.format("Verify expansion %s game card is visible", expansionName)); //4
        logFlowPoint(String.format("Verify expansion %s game card has the '%s' button visible", expansionName, expectedButtonLabel)); //5
        logFlowPoint(String.format("Verify expansion %s game card has the 'Buy as Gift' button visible", expansionName)); //6
        logFlowPoint(String.format("Verify clicking 'Buy As Gift' button at expansion %s game card opens the 'Friends Selection' dialog", expansionName));//7
        logFlowPoint(String.format("Verify addon %s game card is visible", addonName)); //8
        logFlowPoint(String.format("Verify addon %s game card has the 'Unlocked' violator visible", addonName)); //9
        logFlowPoint(String.format("Verify addon %s game card has the 'Buy as Gift' button visible", addonName)); //10
        logFlowPoint(String.format("Verify clicking 'Buy As Gift' button at addon %s game card opens the 'Friends Selection' dialog", addonName));//11
        if (isClient) {
            logFlowPoint(String.format("Click 'Download' at the expansion %s game card", expansionName)); //12 - Client
            logFlowPoint(String.format("Verify expansion %s game card has the 'Installed' violator visible", expansionName)); //13 - Client
            logFlowPoint(String.format("Verify expansion %s game card still has the 'Buy as Gift' button visible", expansionName)); //14- Client
            logFlowPoint(String.format("Verify clicking 'Buy As Gift' button at expansion %s game card again opens the 'Friends Selection' dialog", expansionName));//15 - Client
        }

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as newly registered user: " + username);
        } else {
            logFailExit("Failed: Cannot login as newly registered user: " + username);
        }

        //2
        new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, offerId);
        if (gameTile.waitForDownloadable()) {
            logPass(String.format("Verified successful navigation to Game Library with %s game tile", entitlementName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library or locate %s game tile", entitlementName));
        }

        //3
        gameTile.showGameDetails();
        GameSlideout gameSlideout = new GameSlideout(driver);
        gameSlideout.waitForSlideout();
        boolean slideoutOpened = gameSlideout.verifyGameTitle(entitlementName);
        gameSlideout.clickExtraContentTab();
        SlideoutExtraContent extraContent = new SlideoutExtraContent(driver);
        boolean extraContentOpened = extraContent.verifyBaseGameExtraContent(offerId);
        if (slideoutOpened && extraContentOpened) {
            logPass("Verified Game Slideout 'Extra Content' section opens for " + entitlementName);
        } else {
            String openSlideoutResult = slideoutOpened ? "Can" : "Cannot";
            String openExtraContentResult = extraContentOpened ? "can" : "cannot";
            logFail(String.format("Failed: %s navigate to Game Slideout, %s open 'Extra Content' section for %s",
                    openSlideoutResult, openExtraContentResult, entitlementName));
        }

        final ExtraContentGameCard expansionCard = new ExtraContentGameCard(driver, expansionOfferId);
        final ExtraContentGameCard addonCard = new ExtraContentGameCard(driver, addonOfferId);

        //4
        if (expansionCard.verifyGameCardVisible()) {
            logPass(String.format("Verified 'Extra Content' section has expansion %s game card visible", expansionName));
        } else {
            logFailExit(String.format("Failed: 'Extra Content' section has expansion %s game card visible", expansionName));
        }

        //5
        boolean buttonVisible = isClient
                ? expansionCard.verifyDownloadButtonVisible()
                : expansionCard.verifyInstallOriginAndPlayButtonVisible();
        if (buttonVisible) {
            logPass(String.format("Verified expansion %s game card has the '%s' button visible", expansionName, expectedButtonLabel));
        } else {
            logFailExit(String.format("Failed: Expansion %s game card does not have the '%s' button visible", expansionName, expectedButtonLabel));
        }

        //6
        if (expansionCard.verifyBuyAsGiftButtonVisible()) {
            logPass(String.format("Verified expansion %s game card has the 'Buy as gift' button visible", expansionName));
        } else {
            logFailExit(String.format("Failed: Expansion %s game card does not have the 'Buy as gift' button visible", expansionName));
        }

        //7
        expansionCard.clickBuyAsGiftButton();
        FriendsSelectionDialog friendsSelectionDialog = new FriendsSelectionDialog(driver);
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass(String.format("Verified clicking 'Buy As Gift' button at expansion %s game card opens the 'Friends Selection' dialog", expansionName));
        } else {
            logFailExit(String.format("Failed: Clicking 'Buy As Gift' button at expansion %s game card does not open the 'Friends Selection' dialog", expansionName));
        }
        friendsSelectionDialog.close();

        //8
        if (addonCard.verifyGameCardVisible()) {
            logPass(String.format("Verified 'Extra Content' section has addon %s game card visible", addonName));
        } else {
            logFailExit(String.format("Failed: 'Extra Content' section does not have addon %s game card visible", addonName));
        }

        //9
        if (addonCard.verifyUnlockedViolatorVisible()) {
            logPass(String.format("Verified addon %s game card has the 'Unlocked' button visible", addonName));
        } else {
            logFailExit(String.format("Failed: addon %s game card does not have the 'Unlocked' button visible", addonName));
        }

        //10
        if (addonCard.verifyBuyAsGiftButtonVisible()) {
            logPass(String.format("Verified addon %s game card has the 'Buy as gift' button visible", addonName));
        } else {
            logFailExit(String.format("Failed: addon %s game card does not have the 'Buy as gift' button visible", addonName));
        }

        //11
        addonCard.clickBuyAsGiftButton();
        if (friendsSelectionDialog.waitIsVisible()) {
            logPass(String.format("Verified clicking 'Buy As Gift' button at addon %s game card opens the 'Friends Selection' dialog", addonName));
        } else {
            logFailExit(String.format("Failed: Clicking 'Buy As Gift' button at addon %s game card does not open the 'Friends Selection' dialog", addonName));
        }
        friendsSelectionDialog.closeAndWait();

        if (isClient) {
            //12 - Client
            expansionCard.clickDownloadButton();
            DownloadLanguageSelectionDialog selectionDialog = new DownloadLanguageSelectionDialog(driver);
            selectionDialog.waitForVisible();
            selectionDialog.clickAccept();
            DownloadInfoDialog infoDialog = new DownloadInfoDialog(driver);
            infoDialog.waitForVisible();
            infoDialog.checkPunkBuster();
            infoDialog.clickNext();
            DownloadEULASummaryDialog eulaSummaryDialog = new DownloadEULASummaryDialog(driver);
            eulaSummaryDialog.waitForVisible();
            eulaSummaryDialog.setLicenseAcceptCheckbox(true);
            eulaSummaryDialog.clickNextButton();
            eulaSummaryDialog.waitForClose();
            gameSlideout.clickSlideoutCloseButton();

            if (Waits.pollingWait(() -> new DownloadManager(driver).verifyDownloadCompleteCountEquals("2"), 1000000, 1000, 0)) {
                logPass("Successfully waited for the download to finish");
            } else {
                logFailExit("Failed to wait for the download to finish");
            }

            //13 - Client
            new GameTile(driver, offerId).showGameDetails();
            gameSlideout.waitForSlideout();
            gameSlideout.clickExtraContentTab();
            if (expansionCard.verifyInstalledViolatorVisible()) {
                logPass(String.format("Verified %s game card has the 'Installed' button visible", expansionName));
            } else {
                logFail(String.format("Failed: %s game card does not have the 'Installed' button visible", expansionName));
            }

            //14 - Client
            if (expansionCard.verifyBuyAsGiftButtonVisible()) {
                logPass(String.format("Verified expansion %s game card still has the 'Buy as gift' button visible", expansionName));
            } else {
                logFail(String.format("Failed: Expansion %s game card no longer  have the 'Buy as gift' button visible", expansionName));
            }

            //15 - Client
            expansionCard.clickBuyAsGiftButton();
            if (friendsSelectionDialog.waitIsVisible()) {
                logPass(String.format("Verified clicking 'Buy As Gift' button at expansion %s game card again opens the 'Friends Selection' dialog", expansionName));
            } else {
                logFail(String.format("Failed: Clicking 'Buy As Gift' button at expansion %s game card does not open the 'Friends Selection' dialog", expansionName));
            }
        }

        softAssertAll();
    }
}
