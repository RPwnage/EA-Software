package com.ea.originx.automation.scripts.navigation;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNavigation;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.EntitlementService;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test page navigation using URLs
 *
 * @author rocky
 */
public class OAQuickNavigationViaURL extends EAXVxTestTemplate {

    @Test(groups = {"navigation", "full_regression"})
    public void testQuickNavigationViaURL(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);
        final UserAccount userAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);
        final String entitlementOfferID = entitlement.getOfferId();
        EntitlementService.grantEntitlement(userAccount, entitlementOfferID);

        logFlowPoint("Log into Origin with one entitlement"); // 1
        logFlowPoint("Verify navigation to My Home page"); // 2
        logFlowPoint("Verify navigation to Store page"); // 3
        logFlowPoint("Verify navigation to Browse Games page"); // 4
        logFlowPoint("Verify navigation to Store - Deals page"); // 5
        logFlowPoint("Verify navigation to Store - Deals - On Sale Page page"); // 6
        logFlowPoint("Verify navigation to Store - Deals - Under $5 page"); // 7
        logFlowPoint("Verify navigation to Store - Deals - Under $10 page"); // 8
        logFlowPoint("Verify navigation to Store - Free Games page page"); // 9
        logFlowPoint("Verify navigation to Store - Free Games - On The House page"); // 10
        logFlowPoint("Verify navigation to Store - Free Games - Trials page"); // 11
        logFlowPoint("Verify navigation to Store - Free Games - Demo And Betas page page"); // 12
        logFlowPoint("Verify navigation to Origin Access page"); // 13
        logFlowPoint("Verify navigation to Origin Access - Vault Games page"); // 14
        logFlowPoint("Verify navigation to Origin Access - Play First Trials page"); // 15
        logFlowPoint("Verify navigation to Origin Access - FAQ page"); // 16
        logFlowPoint("Verify navigation to My Game Library page"); // 17
        logFlowPoint("Verify navigation to Origin Game Detail page - Friends Who Play tab"); // 18
        logFlowPoint("Verify navigation to Origin Game Detail page - Achievements tab"); // 19
        logFlowPoint("Verify navigation to Origin Game Detail page - Extra Content tab"); // 20
        logFlowPoint("Verify navigation to user profile page - Achievements tab"); // 21
        logFlowPoint("Verify navigation to user profile page - Friends tab"); // 22
        logFlowPoint("Verify navigation to user profile page - Games tab"); // 23
        logFlowPoint("Verify navigation to user profile page - Wishlist tab"); // 24
        logFlowPoint("Verify navigation to Global Search Results page"); // 25
        if (isClient) {
            logFlowPoint("Verify navigation to Applications Settings page"); // 26
            logFlowPoint("Verify navigation to Diagnostics Settings page"); // 27
            logFlowPoint("Verify navigation to Install And Saves Settings page"); // 28
            logFlowPoint("Verify navigation to Notifications Settings page"); // 29
            logFlowPoint("Verify navigation to Origin In Game Settings page"); // 30
            logFlowPoint("Verify navigation to Voice Settings page"); // 31
        } else {
            logFlowPoint("Verify navigation to About page"); // 26
            logFlowPoint("Verify navigation to Download Origin page"); // 27
        }

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user: " + userAccount.getUsername() + " with entitlement: " + entitlement.getName());
        } else {
            logFailExit("Could not log into Origin with the user with one entitlement.");
        }

        // 2
        MacroNavigation quickNavigation = new MacroNavigation(driver);
        if (quickNavigation.toMyHomePage()) {
            logPass("Load/Navigated to the My Home Page");
        } else {
            logFail("Failed to load/navigate to My Home page");
        }

        //3
        if (quickNavigation.toStorePage()) {
            logPass("Load/Navigated to the Store Page");
        } else {
            logFail("Failed to load/navigate to Store page");
        }

        //4
        if (quickNavigation.toBrowseGamesPage()) {
            logPass("Load/Navigated to the Store - Browse Games Page");
        } else {
            logFail("Failed to load/navigate to Store - Browse Games page");
        }

        //5
        if (quickNavigation.toDealsPage()) {
            logPass("Load/Navigated to the Store - Deals Page");
        } else {
            logFail("Failed to load/navigate to Store - Deals page");
        }

        //6
        if (quickNavigation.toOnSalePage()) {
            logPass("Load/Navigated to the Store - Deals - On Sale Page");
        } else {
            logFail("Failed to load/navigate to Store - Deals - On Sale page");
        }

        //7
        if (quickNavigation.toUnder5Page()) {
            logPass("Load/Navigated to the Store - Deals - Under $5 Page");
        } else {
            logFail("Failed to load/navigate to Store - Deals - Under $5 page");
        }

        //8
        if (quickNavigation.toUnder10Page()) {
            logPass("Load/Navigated to the Store - Deals - Under $10 page");
        } else {
            logFail("Failed to load/navigate to Store - Deals - Under $10 page");
        }

        //10
        //12
        //13
        if (quickNavigation.toOriginAccessPage()) {
            logPass("Load/Navigated to Origin Access page");
        } else {
            logFail("Failed to load/navigate to Origin Access page");
        }

        //14
        if (quickNavigation.toVaultGamesPage()) {
            logPass("Load/Navigated to Origin Access - Vault Games page");
        } else {
            logFail("Failed to load/navigate to Origin Access - Vault Games page");
        }

        //15
        if (quickNavigation.toPlayFirstTrials()) {
            logPass("Load/Navigated to Origin Access - Play First Trials page");
        } else {
            logFail("Failed to load/navigate to Origin Access - Play First Trials page");
        }

        //16
        if (quickNavigation.toOriginAccessFAQ()) {
            logPass("Load/Navigated to Origin Access - FAQ page");
        } else {
            logFail("Failed to load/navigate to Origin Access - FAQ page");
        }

        //17
        if (quickNavigation.toMyGameLibraryPage()) {
            logPass("Load/Navigated to My Game Library page");
        } else {
            logFail("Failed to load/navigate to My Game Library page");
        }

        //18
        if (quickNavigation.toOGDFriendsWhoPlayTab(entitlementOfferID)) {
            logPass("Load/Navigated to Origin Game Detail page - Friends Who Play tab");
        } else {
            logFail("Failed to load/navigate to Origin Game Detail Page - Friends Who Play tab");
        }

        //19
        if (quickNavigation.toOGDAchievementsTab(entitlementOfferID)) {
            logPass("Load/Navigated to Origin Game Detail page - Achievements tab");
        } else {
            logFail("Failed to load/navigate to Origin Game Detail page - Achievements tab");
        }

        //20
        if (quickNavigation.toOGDExtraContentTab(entitlementOfferID)) {
            logPass("Load/Navigated to Origin Game Detail page - Extra Content tab");
        } else {
            logFail("Failed to load/navigate to Origin Game Detail page - Extra Content tab");
        }

        //21
        if (quickNavigation.toProfileAchievementsTab()) {
            logPass("Load/Navigated to user profile page - Achievements tab");
        } else {
            logFail("Failed to load/navigate to user profile Page - Achievements tab");
        }

        //22
        if (quickNavigation.toProfileFriendsTab()) {
            logPass("Load/Navigated to user profile page - Friends tab");
        } else {
            logFail("Failed to load/navigate to user profile Page - Friends tab");
        }

        //23
        if (quickNavigation.toProfileGamesTab()) {
            logPass("Load/Navigated to user profile page - Games tab");
        } else {
            logFail("Failed to load/navigate to user profile Page - Games tab");
        }

        //24
        if (quickNavigation.toProfileWishlistTab()) {
            logPass("Load/Navigated to user profile page - Wishlist tab");
        } else {
            logFail("Failed to load/navigate to user profile Page - Wishlist tab");
        }

        //25
        if (quickNavigation.toGlobalSearchResultsPage(entitlement.getName())) {
            logPass("Load/Navigated to Global Search Results page");
        } else {
            logFail("Failed to load/navigate to Global Search Results Page");
        }

        if (isClient) {
            //26
            if (quickNavigation.toApplicationSettingsPage()) {
                logPass("Load/Navigated to Applications Settings page");
            } else {
                logFail("Failed to load/navigate to Applications Settings page");
            }
            //27
            if (quickNavigation.toDiagnosticsSettingsPage()) {
                logPass("Load/Navigated to Diagnostics Settings page");
            } else {
                logFail("Failed to load/navigate to Diagnostics Settings page");
            }
            //28
            if (quickNavigation.toInstallAndSavesSettingsPage()) {
                logPass("Load/Navigated to Install And Saves Settings page");
            } else {
                logFail("Failed to load/navigate to Install And Saves Settings page");
            }
            //29
            if (quickNavigation.toNotificationsSettingsPage()) {
                logPass("Load/Navigated to Notifications Settings page");
            } else {
                logFail("Failed to load/navigate to Notifications Settings page");
            }
            //30
            if (quickNavigation.toOriginInGameSettingsPage()) {
                logPass("Load/Navigated to Origin In Game Settings page");
            } else {
                logFail("Failed to load/navigate to Origin In Game Settings page");
            }
            //31
            if (quickNavigation.toVoiceSettingsPage()) {
                logPass("Load/Navigated to Voice Settings page");
            } else {
                logFail("Failed to load/navigate to Voice Settings page");
            }
        } else {
            //26
            if (quickNavigation.toAboutStorePage()) {
                logPass("Load/Navigated to About page");
            } else {
                logFail("Failed to load/navigate to About page");
            }
            //27
            if (quickNavigation.toDwnloadOriginPage()) {
                logPass("Load/Navigated to Download Origin page");
            } else {
                logFail("Failed to load/navigate to Download Origin page");
            }
        }
        softAssertAll();
    }
}
