package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.settings.AppSettings;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the memory performance of profile page in order to expose memory leaks
 *
 * @author mdobre
 */
public class OAMemoryPerformanceProfile extends EAXVxTestTemplate {

    @Test(groups = {"profile", "memory_profile"})
    public void testPerformanceProfile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.X_PERFORMANCE);
        userAccount.cleanFriends();
        final UserAccount firstFriendAccount = AccountManager.getRandomAccount();
        final UserAccount secondFriendAccount = AccountManager.getRandomAccount();
        UserAccountHelper.addFriends(userAccount, firstFriendAccount, secondFriendAccount);

        logFlowPoint("Login to Origin with an existing account."); //1
        for (int i = 0; i < 15; i++) {
            logFlowPoint("Navigate to the Application Setting page, to the user's Profile Page and perform garbage collection"); // 2 - 16
        }

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccount.getUsername());
        } else {
            logFailExit("Could not log into Origin with the user " + userAccount.getUsername());
        }

        //2-16
        MainMenu mainMenu = new MainMenu(driver);
        AppSettings appSettings = new AppSettings(driver);
        ProfileHeader profileHeader = new ProfileHeader(driver);
        boolean isAppSettingsPageReached = false;
        boolean isUserProfilePageReached = false;
        boolean isGarbageCollectionSuccesful = false;
        
        for (int i = 0; i < 15; i++) {
            
            //Navigate to Application Settings page
            mainMenu.selectApplicationSettings();
            isAppSettingsPageReached = appSettings.verifyAppSettingsReached();
            
            //Navigate to User's Profile Page
            mainMenu.selectMyProfile();
            isUserProfilePageReached = profileHeader.verifyUsername(userAccount.getUsername());
            
            //Perform Garbage Collection
            isGarbageCollectionSuccesful = garbageCollect(driver, context);
            
            if (isAppSettingsPageReached && isUserProfilePageReached && isGarbageCollectionSuccesful) {
                logPass("Successfully navigated to the Application Setting page, then to the user's Profile Page and performed garbage collection.");
            } else {
                logFail("Failed to navigate to the Application Setting page, then to the user's Profile Page or perform garbage collection.");
            }
        }
        softAssertAll();
    }
}