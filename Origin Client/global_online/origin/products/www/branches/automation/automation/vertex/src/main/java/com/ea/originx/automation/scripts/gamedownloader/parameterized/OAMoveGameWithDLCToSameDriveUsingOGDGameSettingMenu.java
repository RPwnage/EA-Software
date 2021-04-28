package com.ea.originx.automation.scripts.gamedownloader.parameterized;

import com.ea.originx.automation.scripts.gamedownloader.OAMoveGame;
import com.ea.vx.annotations.TestRail;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test move game with DLC to same drive using OGD game setting menu
 *
 * @author sunlee
 */
public class OAMoveGameWithDLCToSameDriveUsingOGDGameSettingMenu extends OAMoveGame {

    @TestRail(caseId = 4383193)
    @Test(groups = {"gamelibrary", "gamedownloader", "client_only"})
    public void testMoveGameWithDLCToSameDriveUsingOGDGameSettingMenu(ITestContext context) throws Exception {
        testMoveGame(context, true, true, METHOD_TO_MOVE_GAME.USE_OGD_GAME_SETTINGS, this.getClass().getName());
    }
}