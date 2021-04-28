package com.ea.originx.automation.scripts.myhome;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroNavigation;
import com.ea.originx.automation.lib.pageobjects.common.MyHomeCarousel;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test carousels on 'My Home' page
 * @author mdobre
 */
public class OAMyHomeCarousel extends EAXVxTestTemplate {

    @TestRail(caseId = 3121266)
    @Test(groups = {"myhome", "services_smoke"})
    public void testMyHomeCarousel(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BF4_STANDARD);

        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);

        logFlowPoint("Login to Origin."); //1
        logFlowPoint("Navigate to the 'My Home' page."); //2
        logFlowPoint("Find a carousel that has more tiles than the standard width of the carousel."); //3
        logFlowPoint("Verify that the tiles have images for their items."); //4
        logFlowPoint("Verify clicking on the right navigation arrow goes to the second page of tiles."); //5
        logFlowPoint("Verify clicking on the left navigation arrow goes to the first page of tiles."); //6
        logFlowPoint("Verify clicking on the second button goes to the second page of tiles."); //7

        //1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged in as " + userAccount.getUsername());
        } else {
            logFailExit("Failed to log in as " + userAccount.getUsername());
        }

        //2
        MacroNavigation quickNavigation = new MacroNavigation(driver);
        if (quickNavigation.toMyHomePage()) {
            logPass("Successfully navigated to the 'My Home' page.");
        } else {
            logFail("Failed to navigate to the 'My Home' page.");
        }

        //3
        MyHomeCarousel myHomeCarousel = new DiscoverPage(driver).getFirstMultiPageCarouselTitle();
        if (myHomeCarousel.verifyCarouselWithBiggerSizeExists()) {
            logPass("Successfully found a carousel that has more tiles than the standard witdth of the carousel.");
        } else {
            logFail("Failed to find a carousel that has more tiles than the standard witdth of the carousel.");
        }

        //4
        if (myHomeCarousel.verifyTilesHaveImages()) {
            logPass("Successfully verified that the tiles have images for their items.");
        } else {
            logFail("Failed to verify that the tiles have images for their items.");
        }

        //5
        boolean isSecondIndicatorUnchecked = !myHomeCarousel.verifyIndicatorChecked(2);
        myHomeCarousel.clickOnRightArrow();
        boolean isSecondIndicatorChecked = myHomeCarousel.verifyIndicatorChecked(2);
        if (isSecondIndicatorChecked && isSecondIndicatorUnchecked) {
            logPass("Successfully verified clicking on the right navigation arrow goes to the second page of tiles.");
        } else {
            logFail("Failed to verify clicking on the right navigation arrow goes to the second page of tiles.");
        }
       
        //6
        boolean isFirstIndicatorUnchecked = !myHomeCarousel.verifyIndicatorChecked(1);
        myHomeCarousel.clickOnLeftArrow();
        boolean isFirstIndicatorChecked = myHomeCarousel.verifyIndicatorChecked(1);
        if (isFirstIndicatorUnchecked && isFirstIndicatorChecked) {
            logPass("Successfully verified clicking on the left navigation arrow goes to the first page of tiles.");
        } else {
            logFail("Failed to verify clicking on the left navigation arrow goes to the first page of tiles.");
        }
            
        //7
        myHomeCarousel.clickOnIndicator(2);
        if (Waits.pollingWait(() -> myHomeCarousel.verifyIndicatorChecked(2))) {
            logPass("Successfully verified clicking on the second button goes to the second page of tiles.");
        } else {
            logFail("Failed to verify clicking on the second button goes to the second page of tiles.");
        }
        softAssertAll();
    }
}