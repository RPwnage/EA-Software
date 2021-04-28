package com.ea.originx.automation.scripts;

import com.ea.originx.automation.lib.helpers.I18NUtil;
import com.ea.originx.automation.lib.pageobjects.template.EAXVxSiteTemplate;
import com.ea.originx.automation.lib.util.LocalFileUtilities;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.templates.OATestBase;
import com.ea.vx.originclient.utils.SystemUtilities;
import java.io.File;
import java.util.Locale;
import org.openqa.selenium.JavascriptExecutor;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;

/**
 * Test template for items we want to use specifically for the EAX project. Will
 * contain methods we want to have available in all the tests.
 *
 * @author glivingstone
 */
public class EAXVxTestTemplate extends OATestBase {

    /**
     * Determines if the Origin client is closed. Note that closed is different
     * from being exited because in the closed state, the Origin client process
     * is still running
     *
     * @param driver The WebDriver for the running instance of Origin
     * @return True if the Origin client is closed
     */
    public static boolean isOriginClientClosed(WebDriver driver) {
        try {
            EAXVxSiteTemplate.switchToOriginWindow(driver);
        } catch (TimeoutException e) {
            return true;
        }
        return false;
    }

    /**
     * Setup performed before each test. Overrides the method in OATestBase to
     * delete a folder if it's local testing.
     *
     * @param context The ITestContext associated with the test
     */
    @BeforeMethod(alwaysRun = true)
    @Override
    protected void testSetUp(ITestContext context) {
        super.testSetUp(context);
        // If local, then Origin installs games to a different location based on
        // the current working directory. This will delete the location if you
        // work on a different drive.
        if (ContextHelper.isLocalHub(context)) {
            String currentDrive = System.getProperty("user.dir").split(":")[0];

            String defaultGamesPath = currentDrive + SystemUtilities.getOriginGamesFolder(true, null).substring(1);
            String defaultx86GamesPath = currentDrive + SystemUtilities.getOriginGamesFolderx86().substring(1);

            LocalFileUtilities.deleteFolder(new File(defaultGamesPath));
            LocalFileUtilities.deleteFolder(new File(defaultx86GamesPath));
        }
    }

    /**
     * This method is run after every test to cleanup leftover processes and
     * make sure the results are sent. This adds the functionality to reset the
     * locale to the default locale after test completion.
     *
     * @param result The results of the test
     * @param context The context in which the driver will be stored in the
     * context attributes.
     */
    @AfterMethod(alwaysRun = true)
    @Override
    protected void testCleanUp(ITestResult result, ITestContext context) {
        I18NUtil.setLocale(Locale.getDefault());
        super.testCleanUp(result, context);
    }

    /**
     * Enter javascript command
     *
     * @param driver webdriver
     * @param command javascript
     */
    protected static void executeJavascriptCommand(WebDriver driver, String command) {
        ((JavascriptExecutor) driver).executeScript(command);
    }
}
