package com.ea.originx.automation.scripts.originaccess;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.dialog.MediaDialog;
import com.ea.originx.automation.lib.pageobjects.originaccess.PlayFirstTrialsPage;
import com.ea.originx.automation.lib.pageobjects.store.MediaCarousel;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the Media Prop component found in the Play First Trial (PFT) landing page.
 *
 * @author esdecastro
 */
public class OAPFTLandingPageMediaProp extends EAXVxTestTemplate {

    @TestRail(caseId = 12101)
    @Test(groups = {"originaccess", "full_regression"})
    public void testPFTLandingPageMediaProp(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManagerHelper.registerNewThrowAwayAccountThroughREST();

        final int firstVideoValue = 0;

        logFlowPoint("Open Origin and log in as a non-subscriber"); // 1
        logFlowPoint("Navigate to the 'Play First Trial' landing page"); // 2
        logFlowPoint("Verify that the Media Prop module appears on the page"); //3
        logFlowPoint("Verify that the title appears"); // 4
        logFlowPoint("Verify that the 'Media Module' carousel appears"); // 5
        logFlowPoint("Verify that the user can scroll through the 'Media Module' carousel"); // 6
        logFlowPoint("Verify that clicking the video within the carousel opens up the full view modal"); // 7
        logFlowPoint("Verify that the user can scroll through the images and videos within the full view modal"); // 8
        logFlowPoint("Verify that clicking the image within the carousel opens up the full view modal"); // 9
        logFlowPoint("Verify that the descriptor text appears"); // 10

        final WebDriver driver = startClientObject(context, client);

        // 1
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        new NavigationSidebar(driver).clickOriginAccessTrialLink();
        PlayFirstTrialsPage pftPage = new PlayFirstTrialsPage(driver);
        logPassFail(pftPage.verifyPlayFirstTrialPageReached(), true);

        // 3
        logPassFail(pftPage.verifyMediaPropVisible(), true);

        // 4
        logPassFail(pftPage.verifyMediaPropTitleVisible(), false);

        // 5
        logPassFail(pftPage.verifyMediaPropMediaCarouselVisible(), true);

        // 6
        MediaCarousel mediaCarousel = new MediaCarousel(driver);
        mediaCarousel.clickRightArrow();
        logPassFail(mediaCarousel.verifyPageSlide(), true);

        // 7
        mediaCarousel.clickLeftArrow();
        mediaCarousel.openItemOnCarousel(firstVideoValue);
        MediaDialog mediaDialog = new MediaDialog(driver);
        logPassFail(mediaDialog.verifyMediaDialogVisible(), true);

        //8
        mediaDialog.clickNextArrow();
        boolean isScrolledRight = mediaCarousel.verifyMediaModalContentVisible(firstVideoValue);
        mediaDialog.clickPrevArrow();
        boolean isScrolledLeft = mediaCarousel.verifyMediaModalContentVisible(firstVideoValue + 1);
        logPassFail(isScrolledRight && isScrolledLeft, false);

        // 9
        mediaDialog.close();
        mediaCarousel.clickFirstImageItem();
        logPassFail(mediaDialog.verifyMediaDialogVisible(), true);

        // 10
        mediaDialog.close();
        logPassFail(pftPage.verifyMediaPropDescriptorTextVisible(), true);

        softAssertAll();
    }
}
