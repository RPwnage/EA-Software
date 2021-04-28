package com.ea.originx.automation.scripts.gdp;

import com.ea.originx.automation.lib.macroaction.MacroGDP;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.MediaDialog;
import com.ea.originx.automation.lib.pageobjects.gdp.GDPSections;
import com.ea.originx.automation.lib.pageobjects.store.MediaCarousel;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.awt.event.KeyEvent;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the GDP media module carousel.
 *
 * @author esdecastro
 */
public class OAMediaModule extends EAXVxTestTemplate {

    @TestRail(caseId = 1016687)
    @Test(groups = {"pdp","full_regression"})
    public void testMediaModule(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount user = AccountManager.getRandomAccount();
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.BATTLEFIELD_1_STANDARD);

        final int firstVideoValue = 0;

        logFlowPoint("Open Origin and log in using a random account");
        logFlowPoint("Navigate to a PDP page that has a media module configured");
        logFlowPoint("Scroll to 'Screenshots and Videos' and verify that the media module is displayed on the page");
        logFlowPoint("Click on a thumbnail and verify that the media modal appears");
        logFlowPoint("Verify that the screenshot or video appears on the modal");
        logFlowPoint("Click on the right arrow and verify that the user is scrolled to the right");
        logFlowPoint("Click on the left arrow key and verify that the user is scrolled to the left");
        logFlowPoint("Press the right arrow key on the key board and verify that the user is scrolled right");
        logFlowPoint("Press the left arrow key on the keyboard and verify that the user is scrolled left");
        logFlowPoint("Go to a video and verify that the user can play the video");
        logFlowPoint("Scroll to another page then scroll back and verify that the video is paused");

        final WebDriver driver = startClientObject(context, client);

        //1
        logPassFail(MacroLogin.startLogin(driver, user), true);

        //2
        logPassFail(MacroGDP.loadGdpPage(driver, entitlement), false);

        //3
        logPassFail(new GDPSections(driver).verifyMediaModuleVisible(), true);

        //4
        MediaCarousel mediaCarousel = new MediaCarousel(driver);
        mediaCarousel.openItemOnCarousel(firstVideoValue);
        MediaDialog mediaDialog = new MediaDialog(driver);
        logPassFail(mediaDialog.verifyMediaDialogVisible(), true);

        //5
        logPassFail(mediaCarousel.verifyMediaModalContentVisible(firstVideoValue), true);

        //6
        mediaDialog.clickNextArrow();
        logPassFail(mediaCarousel.verifyMediaModalContentVisible(firstVideoValue + 1), false);

        //7
        mediaDialog.clickPrevArrow();
        logPassFail(mediaCarousel.verifyMediaModalContentVisible(firstVideoValue), false);

        //8
        RobotKeyboard robotKeyboard = new RobotKeyboard();
        robotKeyboard.pressAndReleaseKeys(client, KeyEvent.VK_RIGHT);
        sleep(500);//wait for the key to be pressed and image to scroll
        logPassFail(mediaCarousel.verifyMediaModalContentVisible(firstVideoValue + 1), false);

        //9
        robotKeyboard.pressAndReleaseKeys(client, KeyEvent.VK_LEFT);
        sleep(500);//wait for the key to be pressed and image to scroll
        logPassFail(mediaCarousel.verifyMediaModalContentVisible(firstVideoValue), false);

        //10
        mediaDialog.playVideo();
        logPassFail(mediaDialog.verifyVideoIsPlaying(),true);

        //11
        driver.switchTo().defaultContent(); // need to jump out of the iframe to click the right button
        sleep(2000); // clicking right too soon after video plays will cause the pause call to not trigger.
        mediaDialog.clickNextArrow();
        logPassFail(mediaDialog.verifyVideoIsPaused(),true);

        softAssertAll();
    }
}
