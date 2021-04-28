package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAMoveGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test move game without DLC to same drive using using OGD game setting menu
 *
 * @author sunlee
 */
public class OAMoveGameWithoutDLCToSameDriveUsingOGDGameSettingMenu extends OAMoveGame {

    @TestRail(caseId = 4383088)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testMoveGameWithoutDLCToSameDriveUsingOGDGameSettingMenu(ITestContext context) throws Exception {
        testMoveGame(context, false, true, METHOD_TO_MOVE_GAME.USE_OGD_GAME_SETTINGS, this.getClass().getName());
    }
}