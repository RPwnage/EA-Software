package com.ea.originx.automation.scripts.gamelibrary;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.FileUtil;
import com.ea.vx.originclient.helpers.RobotKeyboard;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameLibrary;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.dialog.ProblemLaunchingGameMsgBox;
import com.ea.originx.automation.lib.resources.games.Write;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;

import java.lang.invoke.MethodHandles;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.ITestResult;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.Test;

/**
 * Test changing non-origin game launch path
 * (This ticket should be combine with OANonOriginGameSlideout - C11129)
 *
 * @author palui
 */
public class OANonOriginGameLaunchPath extends EAXVxTestTemplate {

    private final String folder1Name = OriginClientConstants.DESKTOP_PATH + "OATEST1";
    private final String folder2Name = OriginClientConstants.DESKTOP_PATH + "OATEST2";

    private final Write nonOriginGame = new Write();
    private final String nonOriginGameName = nonOriginGame.getName();
    private final String nonOriginGameExeName = nonOriginGame.getExecutableName();

    private final String folder1Exe = folder1Name + "\\" + nonOriginGameExeName;
    private final String folder2Exe = folder2Name + "\\" + nonOriginGameExeName;

    private OriginClient CLIENT = null;

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    @TestRail(caseId = 491596)
    @Test(groups = {"gamelibrary", "client_only", "full_regression"})
    public void testNonOriginGameLaunchPath(ITestContext context) throws Exception {

        CLIENT = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String username = userAccount.getUsername();

        logFlowPoint(String.format("Create '%s' & '%s' folders on the desktop. Copy '%s' to '%s'",
                folder1Name, folder2Name, nonOriginGameExeName, folder1Name));//1
        logFlowPoint("Launch Origin and login as newly registered user: " + username);//2
        logFlowPoint(String.format("Select 'Add Non-Origin Games' at the 'Games' menu and add full path of '%s\\%s'. Verified '%s' added to the Game Library",
                folder1Name, nonOriginGameExeName, nonOriginGameName));//3
        logFlowPoint(String.format("Launch and then kill '%s'", nonOriginGameName));//4
        logFlowPoint(String.format("Move '%s' from '%s' to '%s' '", nonOriginGameExeName, folder1Name, folder2Name));//5
        logFlowPoint(String.format("Launch '%s' again and verify it does not launch", nonOriginGameName));//6
        logFlowPoint("Verify 'Problem Launching Game' message box appears");//7
        logFlowPoint("Verify 'Update Shortcut' and 'Remove Game' command links are visible");//8
        logFlowPoint(String.format("Click 'Update Shortcut' and enter full path of '%s\\%s'. Verify '%s' launches automatically",
                folder2Name, nonOriginGameExeName, nonOriginGameName));//9

        //1
        // Kill notepad.exe and delete test folders and their content if already exist
        nonOriginGame.killLaunchedProcess(CLIENT);
        FileUtil.deleteFolder(CLIENT, folder1Name);
        FileUtil.deleteFolder(CLIENT, folder2Name);

        boolean createFolders = FileUtil.createFolder(CLIENT, folder1Name) && FileUtil.createFolder(CLIENT, folder2Name);
        FileUtil.copyFile(CLIENT, nonOriginGame.getExecutablePath(CLIENT), folder1Exe);
        if (createFolders && FileUtil.isFileExist(CLIENT, folder1Exe)) {
            logPass(String.format("Verified file '%s\\%s' and folder '%s' successfully created", folder1Name, nonOriginGameExeName, folder2Name));
        } else {
            logFailExit(String.format("Failed: Cannot create file '%s\\%s' and/or folder '%s'",
                    folder1Name, nonOriginGameExeName, folder2Name));
        }

        //2
        WebDriver driver = startClientObject(context, CLIENT);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + username);
        } else {
            logFailExit("Failed: Cannot login as user: " + username);
        }

        //3
        new MainMenu(driver).selectAddNonOriginGame();
        new RobotKeyboard().typeThenHitEnter(CLIENT, folder1Exe);
        GameLibrary gameLibrary = new NavigationSidebar(driver).gotoGameLibrary();
        GameTile gameTile = new GameTile(driver, gameLibrary.getGameTileOfferIdByName(nonOriginGameName));
        if (gameTile.isGameTileVisible()) {
            logPass(String.format("Verified successful navigation to Game Library showing '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot navigate to Game Library, or '%s' not shown", nonOriginGameName));
        }

        //4
        gameTile.play();
        boolean launched = Waits.pollingWaitEx(() -> nonOriginGame.isLaunched(CLIENT), 30000, 1000, 0);
        sleep(1000); // pause before killing process
        boolean killed = nonOriginGame.killLaunchedProcess(CLIENT);
        if (launched && killed) {
            logPass(String.format("Verified successful launch and termination of '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot launch and then terminate '%s'", nonOriginGameName));
        }

        //5
        FileUtil.moveFile(CLIENT, folder1Exe, folder2Exe);
        if (!FileUtil.isFileExist(CLIENT, folder1Exe) && FileUtil.isFileExist(CLIENT, folder2Exe)) {
            logPass(String.format("Verified '%s' successfully moved from '%s' to '%s'", nonOriginGameExeName, folder1Name, folder2Name));
        } else {
            logPass(String.format("Failed: Cannot move '%s' from '%s' to '%s'", nonOriginGameExeName, folder1Name, folder2Name));
        }

        //6
        gameTile.play();
        if (Waits.pollingWaitEx(() -> !nonOriginGame.isLaunched(CLIENT))) {
            logPass(String.format("Verified %s' cannot be launched", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: '%s' can still be launched", nonOriginGameName));
        }

        //7
        ProblemLaunchingGameMsgBox problemLaunchMsgBox = new ProblemLaunchingGameMsgBox(driver);
        if (Waits.pollingWait(() -> problemLaunchMsgBox.verifyVisible(nonOriginGameName))) {
            logPass(String.format("Verified 'Problem Launching Game' message box appears with the expected titles and message for '%s'", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: 'Problem Launching Game' message box does not appear or has unexpected titles and message for '%s'", nonOriginGameName));
        }

        //8
        boolean updateShortcutLinkVisible = problemLaunchMsgBox.verifyUpdateShortcutCommandLink();
        boolean removeGameLinkVisible = problemLaunchMsgBox.verifyRemoveGameCommandLink();
        if (updateShortcutLinkVisible && removeGameLinkVisible) {
            logPass("Verified 'Update Shortcut' and 'Remove Game' command links are visible");
        } else {
            logFailExit("Failed: 'Update Shortcut' and/or 'Remove Game' command links are not visible");
        }

        //9
        problemLaunchMsgBox.clickUpdateShortcutCommandLink();
        new RobotKeyboard().typeThenHitEnter(CLIENT, folder2Exe);
        if (Waits.pollingWaitEx(() -> nonOriginGame.isLaunched(CLIENT), 30000, 1000, 0)) {
            logPass(String.format("Verified successful launch of '%s' after shortcut updated", nonOriginGameName));
        } else {
            logFailExit(String.format("Failed: Cannot launch '%s' after shortcut updated", nonOriginGameName));
        }
        nonOriginGame.killLaunchedProcess(CLIENT);

        softAssertAll();
    }

    /**
     * Cleanup after script exits
     *
     * @param result  (@link ITestResult)
     * @param context (@link ITestContext)
     */
    @Override
    @AfterMethod(alwaysRun = true)
    protected void testCleanUp(ITestResult result, ITestContext context) {
        if (null != CLIENT) {
            nonOriginGame.killLaunchedProcess(CLIENT);
            FileUtil.deleteFolder(CLIENT, folder1Name);
            FileUtil.deleteFolder(CLIENT, folder2Name);
        }

        _log.debug("Performing test cleanup for " + this.getClass().getSimpleName());
        super.testCleanUp(result, context);
    }
}
