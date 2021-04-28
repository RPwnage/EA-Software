package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAMoveGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test move game without DLC to different drive using OGD game setting menu
 *
 * @author sunlee
 */
public class OAMoveGameWithoutDLCToDifferentDriveUsingOGDGameSettingMenu extends OAMoveGame {

    @TestRail(caseId = 4383159)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testMoveGameWithoutDLCToDifferentDriveUsingOGDGameSettingMenu(ITestContext context) throws Exception {
        testMoveGame(context, false, false, METHOD_TO_MOVE_GAME.USE_OGD_GAME_SETTINGS, this.getClass().getName());
    }
}